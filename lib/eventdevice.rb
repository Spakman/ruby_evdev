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

require 'eventdevice_c'

module Evdev

# This class wraps the Linux evdev interface.  This class inherits from File and
# adds functionality specific to evdev input devices (found at
# /dev/input/event*).  All the functions in this interface have the potential
# to raise an IOerror.
# 
# Look here for more info on the evdev interface itself:
# http://www.linuxjournal.com/article/6429
# 
#   require 'evdev'
#   
#   t = Evdev::EventDevice.open("/dev/input/event3" , "a+") 
#   puts "interface Version is #{t.event_interface_version}"
#   puts "bustype is #{t.bus_type_code}"
#   puts "bustype is #{t.bus_type_name}"
#   puts "vendor is #{t.vendor}"
#   puts "product is #{t.product}"
#   puts "version is #{t.version}"
#   puts "name is #{t.device_name}"
#   puts "topology is #{t.topology}"
#   puts "uniqueid is #{t.unique_id}"
#   puts "activated keys: #{t.activated_keys().inspect}"
#   puts t.write_event(17,2,1); # turn scrollock led on
#   puts "activated leds: #{t.activated_leds().inspect}"
#   puts t.parameters_for_axis(0).inspect
#   t.supported_feature_types.each { | feature |
#     begin 
#       puts "feature #{feature}: #{feature.supported_features.join(', ') }"
#     rescue IOError
#     end
#   }
#   while(1)
#     puts t.read_event.inspect
#   end
#   t.close
# 
class EventDevice < File

  # Returns the name of the bus type this device is connected through.
  def bus_type_name
    return CodeMappings.bus_type(bus_type_code())
  end

  # returns the FeatureType with this name (check out CodeMappings for the
  # FeatureType names).  If this feature is not supported by this device, this
  # function raises.
  def feature_type_named(name)
    result = feature_type_named?(name)
    raise("no feature named #{name} for device #{path}") unless result
    return result
  end

  # returns the feature_type if the device has a feature with this name
  # nil otherwise
  def feature_type_named?(name)
    return self.supported_feature_types.detect { | feature |
      feature.name == name
    }
  end

  # creates an Event object.  Used internally by the C side of EventDevice
  def create_event(type, code, value, time)
    feature_type = create_feature_type(type)
    feature = Feature.new(feature_type, code)
    return Event.new(feature, value, time)
  end

  # creates an FeatureType object.  Used internally by the C side of EventDevice
  def create_feature_type(code)
    return FeatureType.new(self, code)
  end
end

# A simple class generated by EventDevice#read.  Contains the feature that had
# the event, the value of the feature at that event, and the time when the event
# occured.
class Event
  attr_reader :feature,:value,:time;
  def initialize(feature, value, time = nil)
    @feature = feature
    @value = value
    @time = time
  end
end

end # end module Evdev