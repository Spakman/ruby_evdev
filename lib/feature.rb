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

require 'codemappings'

module Evdev

# an object corresponding to one type of input/output for a device.  All leds,
# buttons, keys etc. are grouped together into a FeatureType.  This type object
# can be queried to determine what features it supports.
class FeatureType
  attr_reader :device, :code
  def initialize(device, code)
    @device = device
    @code = code
  end

  # Returns the name of the feature type.  Check out CodeMappings for the
  # available names
  def name
    return CodeMappings.feature_type_name(@code)
  end

  def to_s
    return "<FeatureType #{name()} (#{@code})>"
  end
  
  # Returns the list of Feature objects that this FeatureType supports
  def supported_features
    @device.supported_features_for_type(@code).collect {|i| Feature.new(self, i) }
  end

  # Returns the Feature within this type for the given name (check out
  # CodeMappings for the list of possible names).  This will throw if the
  # Feature is not supported by this device (even if it's one of the possible
  # values for the FeatureType)
  def feature_named(name)
    result = feature_named?(name)
    raise("feature type #{self.name} does not have a feature #{name}") unless result
    return result
  end
  
  # returns the feature if it is supported by this device, nil otherwise
  def feature_named?(name)
    return supported_features.detect { | feature | feature.name == name }
  end
  # Returns a list of all the Features within this type that are currently
  # activated (that is, on for leds, depressed for keys, playing for sounds).
  # This is only supported for the KEY LED and SND feature types.  It will raise
  # for other types.
  def activated_features
    if(name() == 'KEY')
      return features_from_list(@device.activated_keys)
    elsif(name() == 'LED')
      return features_from_list(@device.activated_leds)
    elsif(name() == 'SND')
      return features_from_list(@device.activated_sounds)
    else
      raise "No method to get activated features for type #{name()}"
    end
  end

  def features_from_list(aList) # :nodoc:
    return aList.collect {|i| Feature.new(self, i) }
  end
end

# An object corresponding to a single key, led, mouse axis, etc.
class Feature
  attr_reader :type, :code
  def initialize(type, code)
    @type = type
    @code = code
  end

  # Returns the name of the feature.  Check out CodeMappings for the
  # available names
  def name
    return CodeMappings.feature_name(@type.code, @code) 
  end

  def to_s
    return "<Feature #{CodeMappings.feature_name(@type.code, @code)}(#{@code})>"
  end

  #   EventDevice.open(ARGV[0] , "a+") { |device|
  #     led = device.feature_type_named('LED').feature_named('SCROLLL')
  #     led.write_value(1)
  #   }
  # Writes a value to the device for the given feature type and feature.  For
  # LEDs writing 0/1 turns them on and off.  Exactly how the device responds to
  # this writing is up to the device driver writer.
  def write_value(value)
    @type.device.write_event(@type.code, @code, value)
  end

  # Returns true if the feature is currently activated, false otherwise.  This
  # query is only supported on KEY,LED, and SND feature_types.  This function
  # will raise if the feature_type is not one of those three.
  def activated?()
    return true if @type.activated_features.detect { |i| i.code == @code }
    return false
  end
end

end # module Evdev
