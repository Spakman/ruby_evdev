#!/usr/bin/ruby
# A sample app highlighting one of the more amusing features of the evdev
# library - the ability to control keyboard LEDs.  The utility takes data on
# STDIN and ouputs it to the scoll lock LED of the given event device - in morse
# code.  I think you could use this same functionality to do something a lot
# more useful (say, flash when you've got mail).
#
# == Usage
#
# morseout.rb [--find-devices | -f]
#
# morseout.rb --device /dev/input/eventX
#
# morseout.rb --help | -h
#
# [device] One of the evdev devices in /dev/input/event*.  It has to have an LED
#          feature_type and the SCROLL feature.  This basically means a keyboard device.
#          To figure out which devices support this, use the --find-devices option
# [find-devices]
#          lists the path and name of all devices that seem to support the
#          required interface in /dev/input/event*.  This is the default
#          behavior if no other options are specified.
#
# == Examples
#
# echo 'SOS' | sudo ./morseout.rb --device /dev/input/event3
# 
# sudo ./morseout.rb --find-devices

#--
#  Copyright 2006, Michael Hewner
#  
#  This is a part of ruby evdev
# 
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, version 2 of the License, 
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#++


require 'evdev'
require 'optparse'
require 'rdoc/usage'


$sound_quanta = 0.12 # the length of one dot, on which everythig is keyed

$mapping = Hash.new
$mapping.default = []
$mapping['a'] = [:dot,:dash]
$mapping['b'] = [:dash, :dot, :dot, :dot]
$mapping['c'] = [:dash, :dot, :dash, :dot]
$mapping['e'] = [:dot]
$mapping['f'] = [:dot,:dot,:dash,:dot]
$mapping['g'] = [:dash,:dash,:dot]
$mapping['h'] = [:dot,:dot,:dot,:dot]
$mapping['i'] = [:dot,:dot]
$mapping['j'] = [:dot,:dash,:dash,:dash]
$mapping['k'] = [:dot,:dash,:dot]
$mapping['l'] = [:dot,:dash,:dot,:dot]
$mapping['m'] = [:dash,:dash]
$mapping['n'] = [:dash,:dot]
$mapping['o'] = [:dash,:dash,:dash]
$mapping['p'] = [:dot,:dash,:dash,:dot]
$mapping['q'] = [:dash,:dash,:dot,:dash]
$mapping['r'] = [:dot,:dash,:dot]
$mapping['s'] = [:dot,:dot,:dot]
$mapping['t'] = [:dash]
$mapping['u'] = [:dot,:dot,:dash]
$mapping['v'] = [:dot,:dot,:dot,:dash]
$mapping['w'] = [:dot,:dash,:dash]
$mapping['x'] = [:dash,:dot,:dot,:dash]
$mapping['y'] = [:dash,:dot,:dash,:dash]
$mapping['z'] = [:dash,:dash,:dot,:dot]
$mapping['0'] = [:dash,:dash,:dash,:dash,:dash]
$mapping['1'] = [:dot,:dash,:dash,:dash,:dash]
$mapping['2'] = [:dot,:dot,:dash,:dash,:dash]
$mapping['3'] = [:dot,:dot,:dot,:dash,:dash]
$mapping['4'] = [:dot,:dot,:dot,:dot,:dash]
$mapping['5'] = [:dot,:dot,:dot,:dot,:dot]
$mapping['6'] = [:dash,:dot,:dot,:dot,:dot]
$mapping['7'] = [:dash,:dash,:dot,:dot,:dot]
$mapping['8'] = [:dash,:dash,:dash,:dot,:dot]
$mapping['9'] = [:dash,:dash,:dash,:dash,:dot]
$mapping['.'] = [:dot,:dash,:dot,:dash,:dot,:dash]
$mapping[' '] = [:word_pause]
$mapping["\n"] = [:word_pause, :dot,:dash,:dot,:dash,:dot,:dash]
$mapping[','] = [:dash,:dash,:dot,:dot,:dash,:dash]
$mapping['?'] = [:dot,:dot,:dash,:dash,:dot,:dot]

def morse_for_character(c)
  result = $mapping[c]
  return result + [:letter_pause]
end

def flash_led(seconds)
  $led.write_value(1)
  sleep(seconds)
  $led.write_value(0)
end

def display_code(c)
  if(c == :dot)
    flash_led($sound_quanta)
    sleep($sound_quanta)
  elsif(c == :dash)
    flash_led($sound_quanta*3)
    sleep($sound_quanta)
  elsif(c == :letter_pause)
    sleep($sound_quanta*3)
  elsif(c == :word_pause)
    sleep($sound_quanta*7)
  else
    raise("unknown code #{c} passed to display_code")
  end
end

def find_devices()
  filenames = Dir.glob('/dev/input/event*')
  filenames.each { |file|
    Evdev::EventDevice.open(file) { |device|
      begin
        device.feature_type_named('LED').feature_named('SCROLLL')
        puts "#{file} (#{device.device_name})"
      rescue StandardError => e
        # if a exception was raised, the device does not support this feature
      end
    }
  }
end
device_path = nil
opts = OptionParser.new
opts.on("-h","--help") { RDoc::usage; exit(0) }
opts.on("-f","--find-devices") { find_devices; exit(0) }
opts.on("-e","--device String") { |str| device_path = str }
opts.parse(ARGV)

unless(device_path)
  puts "You must specify a device (e.g. morseout.rb --device /dev/input/event3)"
  puts "For more help pass --help"
  puts "These devices seem to support the proper features:"
  find_devices()
  exit(1)
end

device = Evdev::EventDevice.open(device_path , "a+")
puts "using #{device.device_name}"
$led = device.feature_type_named('LED').feature_named('SCROLLL')
puts "led currently #{$led.activated? ? 'active' : 'inactive'}"

while(string = STDIN.gets)
  string.downcase!
  for i in 0...string.length
    morse_array = morse_for_character(string[i,1])
    morse_array.each { | code | display_code(code) }
  end
end
