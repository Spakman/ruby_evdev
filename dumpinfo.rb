#!/usr/bin/ruby

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

require 'evdev'

t = Evdev::EventDevice.open(ARGV[0] , "a+") # "/dev/input/event3")
puts "interface Version is #{t.event_interface_version}"
puts "bustype is #{t.bus_type_code}"
puts "bustype is #{t.bus_type_name}"
puts "vendor is #{t.vendor}"
puts "product is #{t.product}"
puts "version is #{t.version}"
puts "name is #{t.device_name}"
# puts "topology is #{t.topology}"
# puts "uniqueid is #{t.unique_id}"
puts "activated keys: #{t.activated_keys().inspect}"
puts "led.."
# puts t.write_event(17,0,1);
# puts t.write_event(17,1,1);
 puts t.write_event(17,2,1);
puts "activated leds: #{t.activated_leds().inspect}"
puts "activated sounds: #{t.activated_sounds().inspect}"
# puts "repeat rate: #{t.repeat_rate.inspect}"
# t.set_repeat_rate(2500,1000);
puts t.parameters_for_axis(0).inspect
puts t.parameters_for_axis(1).inspect
# puts "Scancode for 1: #{t.scancode_for_keycode(1)}"
# t.set_scancode_for_keycode(203,205)
t.supported_feature_types.each { | feature |
  begin 
    puts "feature #{feature}: #{feature.supported_features.join(', ') }"
  rescue IOError
  end
}
while(1)
  puts t.read_event.inspect
end
t.close
