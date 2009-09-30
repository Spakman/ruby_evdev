/*  Copyright 2006, Michael Hewner
 *  
 *  This is a part of ruby evdev
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 2 of the License, 
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <stdio.h>
#include <fcntl.h>
#include <linux/input.h>
#include <unistd.h>
#include <string.h>
#include "ruby.h"
#include "rubyio.h"

#define BITS_PER_LONG (sizeof(long) * 8)
#define NBITS(x) ((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x)  ((x)%BITS_PER_LONG)
#define BIT(x)  (1UL<<OFF(x))
#define LONG(x) ((x)/BITS_PER_LONG)
#define test_bit(bit, array)  ((array[LONG(bit)] >> OFF(bit)) & 1)

VALUE cEventDevice;

static void raise_for_ioctl_error(int fd, char* request_string, int request)
{
  switch (errno) {
    case EBADF:
      rb_raise(rb_eIOError, "file descriptor %d is invalid (errno EBADF %d)", fd, errno);
      break;
    case EFAULT:
      rb_raise(rb_eIOError, 
        "attempted to pass ioctl invalid memory location.  this should not happen. (errno EFAULT %d)", errno);
      break;
    case EINVAL:
      rb_raise(rb_eIOError, 
        "invalid request %s (%d) or arguments (errno EINVAL %d)",
	request_string,
	request,
	errno);
      break;
    case ENOTTY:
      rb_raise(rb_eIOError,
        "either fd %d is not a special device or request %s (%d) is not valid for this kind of device (errno ENOTTY %d)",
	fd,
	request_string,
	request,
	errno);
      break;
    default:
      rb_raise(rb_eIOError,
        "ioctl returned an error and errno is set to unknown code %d (fd %d, request %s %d)",
	errno,
	fd,
	request_string,
	request);
  }
  return;
}

static int get_fd(VALUE self)
{
#if defined(HAVE_RUBY_IO_H) /* seems like Ruby > 1.8 */
  rb_io_t *fptr;

  GetOpenFile(self, fptr);
  return fptr->fd;
#else
  OpenFile *fptr;

  GetOpenFile(self, fptr);
  return fileno(fptr->f);
#endif
}

static struct input_id get_input_info(VALUE self)
{
  int fd = get_fd(self);
  struct input_id input_info;

  if (ioctl(fd, EVIOCGID, &input_info) < 0) {
    raise_for_ioctl_error(fd, "EVIOCGID", EVIOCGID);
  }
  return input_info;
}

/* call-seq:
 *  event_interface_version() -> aString
 * 
 * Returns a string of the form x.y.z idenifing the version of the evdev version
 * being used.
 */
static VALUE ed_event_interface_version(VALUE self)
{
  int fd = get_fd(self);
  int version;
  char version_str[12];

  if (ioctl(fd, EVIOCGVERSION, &version) < 0) {
    raise_for_ioctl_error(fd, "EVIOCGVERSION", EVIOCGVERSION);
  }
  sprintf(version_str, "%d.%d.%d",
  version >> 16, (version >> 8) & 0xff, version & 0xff);
  return rb_str_new2(version_str);
}

/* call-seq:
 *   bus_type_code() -> anInteger
 *
 * Returns a number representing the bus that this device is connected on.
 * Consider bus_type_name
 */
static VALUE ed_bus_type_code(VALUE self)
{
  return INT2FIX(get_input_info(self).bustype);
}

/* call-seq:
 *   vendor() -> anInteger
 *
 * Returns a number representing the manufacturer of the device.
 */
static VALUE ed_vendor(VALUE self)
{
  return INT2FIX(get_input_info(self).vendor);
}

/* call-seq:
 *   product() -> anInteger
 *
 * Returns a number representing the product of the device.
 */
static VALUE ed_product(VALUE self)
{
  return INT2FIX(get_input_info(self).product);
}

/* call-seq:
 *   version() -> anInteger
 *
 * Returns a number representing the version of the device.
 */
static VALUE ed_version(VALUE self)
{
  return INT2FIX(get_input_info(self).version);
}

/* call-seq:
 *   device_name() -> String
 *
 * Returns a human-readable name describing the device.
 */
static VALUE ed_device_name(VALUE self)
{
  int fd = get_fd(self);
  char name[256];

  if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) < 0) {
    raise_for_ioctl_error(fd, "EVIOCGNAME", EVIOCGNAME(sizeof(name)));
  }
  return rb_str_new2(name);
}

/* call-seq:
 *   topology() -> aString
 *
 * Returns a string identifing this object's place in the device topology.  It
 * often looks like this "usb-0000:00:1d.7-1.2.1/input0".  Not all devices
 * support this call in which case this function will raise an IOerror.
 */
static VALUE ed_topology(VALUE self)
{
  int fd = get_fd(self);
  char top[256];

  if (ioctl(fd, EVIOCGPHYS(sizeof(top)), top) < 0) {
    raise_for_ioctl_error(fd, "EVIOCGPHYS", EVIOCGPHYS(sizeof(top)));
  }
  return rb_str_new2(top);
}

/* call-seq:
 *   unique_id() -> string
 *
 * Returns a string that in theory is set by the manufacturer to uniquely
 * identify the device.  In pratice, this returns empty string for most devices
 * and sometimes just raises an IOError outright.
 */
static VALUE ed_unique_id(VALUE self)
{
  int fd = get_fd(self);
  char unique[256];

  if (ioctl(fd, EVIOCGUNIQ(sizeof(unique)), unique) < 0) {
    raise_for_ioctl_error(fd, "EVIOCGUNIQ", EVIOCGUNIQ(sizeof(unique)));
  }
  return rb_str_new2(unique);
}

/* call-seq:
 *   supported_feature_types() -> arrayOfFeatureTypes
 *
 *    require 'evdev'
 *  
 *    Evdev::EventDevice.open("/dev/input/event3") { |device|
 *      types = device.supported_feature_types
 *      names = types.collect { |i| i.name }
 *      puts "Features: #{names.join(', ')}"
 *    }
 * This function returns all the FeatureTypes that the device reports that it
 * supports.
 */
static VALUE ed_supported_feature_types(VALUE self)
{
  int feature_type;
  int fd = get_fd(self);
  VALUE result_array = rb_ary_new();
  long feature_type_bits[NBITS(EV_MAX)];
  //printf("1\n");

  memset(feature_type_bits, 0, sizeof(feature_type_bits));
 // printf("2\n");
  if (ioctl(fd, EVIOCGBIT(0, EV_MAX), feature_type_bits) < 0) {
//  printf("3\n");
    raise_for_ioctl_error(fd, "EVIOCGBIT", EVIOCGBIT(0, EV_MAX));
  }
//  printf("4\n");
  for (feature_type = 0; feature_type < EV_MAX; feature_type++) {
 // printf("5\n");
    if (test_bit(feature_type, feature_type_bits)) { 
 // printf("6\n");
      VALUE feature_type_obj = rb_funcall(self,
		         rb_intern("create_feature_type"),
			 1,
			 INT2NUM(feature_type));
 // printf("7\n");
      rb_ary_push(result_array, feature_type_obj);
    }
  }
 // printf("8\n");
  return result_array;
}

/* call-seq:
 *   read_event() -> Event
 *  
 *    require 'evdev'
 *     
 *    t = Evdev::EventDevice.open("/dev/input/event3") 
 *    while(1)
 *      puts t.read_event.inspect
 *    end
 *    t.close
 *
 *   This function reads a event from the event device.  Note that evdev events
 *   only occur when something changes so this is a call that can block for a
 *   while.  If you want to know the status of the device, as opposed to being
 *   notified about changes use the activated_keys (or activated_leds or
 *   activated_sounds) methods.  This function uses IO#sysread under the covers so
 *   the usual warnings about not combining it with other types of reads
 *   applies.
 */
static VALUE ed_read_event(VALUE self)
{
  struct input_event *event;
  
  VALUE result = rb_funcall(self,rb_intern("sysread"),1, INT2FIX(sizeof(struct input_event)));
  event = (struct input_event*) RSTRING_PTR(result);
  VALUE event_obj = rb_funcall(self,
		         rb_intern("create_event"),
			 4,
                         UINT2NUM(event->type),
                         UINT2NUM(event->code),
                         INT2NUM(event->value),
                         rb_time_new(event->time.tv_sec, event->time.tv_usec));

  return event_obj;
}

/* call-seq:
 *   write_event(int_feature_type, int_feature_code, int_value) -> int
 *
 *   require 'evdev'
 *
 *   Evdev::EventDevice.open("/dev/input/event3" , "a+") { |t|
 *     # 17 is the LED device, 2 is the Scrolllock LED, 1 sets it on
 *     puts t.write_event(17,2,1);
 *   }
 * This function allows you to write to the evdev device.  For LEDs, this turns
 * them on or off.  It's cleaner to use the Feature#write_value function to
 * accomplish this.  The function returns the number of bytes written.  This
 * function uses syswrite under the covers, so the usual rules about combining
 * this with other types of write functions apply. */
static VALUE ed_write_event(VALUE self, VALUE type, VALUE code, VALUE value)
{
  struct input_event event;
  event.type = NUM2UINT(type);
  event.code = NUM2UINT(code);
  event.value = NUM2INT(value);
  VALUE stringFromStruct = rb_str_new(&event, sizeof(struct input_event));
  return rb_funcall(self,rb_intern("syswrite"),1, stringFromStruct);
}

static VALUE ed_activated_stuff(VALUE self, int command)
{
  int feature;
  int fd = get_fd(self);
  VALUE result_array = rb_ary_new();
  long feature_bits[NBITS(KEY_MAX)];

  memset(feature_bits, 0, sizeof(feature_bits));
  if (ioctl(fd, command, feature_bits) < 0) {
    raise_for_ioctl_error(fd, "EVIOCGsomething", command);
  }
  for (feature = 0; feature < KEY_MAX; feature++) {
    if (test_bit(feature, feature_bits)) { 
      rb_ary_push(result_array, INT2FIX(feature));
    }
  }
  return result_array;
}

/* call-seq:
 *   activated_keys() -> aArrayOfCodes
 *
 * This function returns a list of the currently active (that is depressed) key
 * feature codes for a device that has the KEY feature_type.  Otherwise, it
 * returns an empty list.  Better to use FeatureType#activated_features or
 * Feature#activated?
 */
static VALUE ed_activated_keys(VALUE self)
{
  return ed_activated_stuff(self,EVIOCGKEY(KEY_MAX));
}

/* call-seq:
 *   activated_leds() -> aArrayOfCodes
 *
 * This function returns a list of the currently active led feature codes for
 * a device that has the LED feature_type.  Otherwise, it returns an empty
 * list.  Better to use FeatureType#activated_features or Feature#activated?
 */
static VALUE ed_activated_leds(VALUE self)
{
  return ed_activated_stuff(self,EVIOCGLED(KEY_MAX));
}

/* call-seq:
 *   activated_sounds() -> aArrayOfCodes
 *
 * This function returns a list of the currently active sound feature codes for
 * a device that has the SND feature_type.  Otherwise, it returns an empty list.
 * Better to use FeatureType#activated_features or Feature#activated?
 */
static VALUE ed_activated_sounds(VALUE self)
{
  return ed_activated_stuff(self,EVIOCGSND(KEY_MAX));
}

/* call-seq:
 *   supported_features_for_type(feature_type_code) -> aArrayOfCodes
 *
 * This returns an array of codes corresponding to the features of this
 * featuretype that this particular device supports.  Rather than calling this
 * (pretty crude) function, use FeatureType#supportedFeatures
 */
static VALUE ed_supported_features_for_type(VALUE self, VALUE type)
{
  int feature_type = NUM2INT(type);
  return ed_activated_stuff(self, EVIOCGBIT(feature_type, KEY_MAX));
}

/* call-seq:
 *   repeat_rate() -> aHash
 *
 * For key devices that support it, this function returns a hash with the repeat
 * rate settins.  There are two values in the hash:
 * [initial_delay]   this is the amount of time you must hold down a key before it
 *                   begins to repeat
 * [repeat_interval] once the initial_delay is exhausted, a new key event will
 *                   be registed every repeat_interval seconds
 * I'm not sure if this functionality really works, because I've
 * never actually found a keyboard that supports it.  Also note that this
 * functionality is new in Linux2.6 and so older versions of the kernel will
 * just raise right away if you call this function.  If the functionality is
 * available in the kernel but not supported by this particular device, an
 * IOException will be raised.
 */
static VALUE ed_repeat_rate(VALUE self)
{
#ifdef EVIOCGREP
  int fd = get_fd(self);
  int rate[2];

  if (ioctl(fd, EVIOCGREP, rate) < 0) {
    raise_for_ioctl_error(fd, "EVIOCGREP", EVIOCGREP);
  }
  VALUE results_hash = rb_hash_new();
  rb_hash_aset(results_hash, rb_str_new2("intial_delay"), INT2NUM(rate[0]));
  rb_hash_aset(results_hash, rb_str_new2("repeat_interval"), INT2NUM(rate[1]));
  return results_hash;
#else
    rb_raise(rb_eIOError, "your version of linux does not support repeat rate");
#endif
}

/* call-seq:
 *   set_repeat_rate(initial, repeat) -> nil
 *   
 * For key device drivers that support it, you can set the repeat rate
 * parameters.  I'm not sure if this functionality really works, because I've
 * never actually found a keyboard that supports it.  Also note that this
 * functionality is new in Linux2.6 and so older versions of the kernel will
 * just raise right away if you call this function.  If the functionality is
 * available in the kernel but not supported by this particular device, an
 * IOException will be raised.
 */
static VALUE ed_set_repeat_rate(VALUE self, VALUE initial, VALUE repeat)
{
#ifdef EVIOCSREP
  int fd = get_fd(self);
  int rate[2];
  rate[0] = NUM2INT(initial);
  rate[1] = NUM2INT(repeat);

  if (ioctl(fd, EVIOCSREP, rate) < 0) {
    raise_for_ioctl_error(fd, "EVIOCSREP", EVIOCSREP);
  }
  return Qnil;
#else
    rb_raise(rb_eIOError, "your version of linux does not support repeat rate");
#endif
}

/* call-seq:
 *   parameters_for_axis(absolute_axis_feature_code) -> aHash
 *
 *      require 'evdev'
 *      
 *      Evdev::EventDevice.open('/dev/input/event1') { |foo|
 *        axis = foo.feature_type_named('ABS').feature_named('X')
 *        puts foo.parameters_for_axis(axis.code)['value']
 *      }
 *   For devices with the absolute axis feature type (touchscreens, for example)
 *   this function returns parameters about the axis features.  Five values are
 *   returned:
 *
 *   [value] the current value for the axis
 *   [minimum] the minimum value for the axis
 *   [maximum] the maximum value for the axis
 *   [flat] size of the flat section (not really sure what that means)
 *   [fuzz] amount of error that may be present
 *   
 *   This is functionality that only makes sense for devices with the absolute
 *   axis input type, but it will return success on others - coming back with
 *   0s for all values.
 */
static VALUE ed_parameters_for_axis(VALUE self, VALUE axis)
{
  int fd = get_fd(self);
  int axis_val = NUM2INT(axis);
  struct input_absinfo axis_info;

  if (ioctl(fd, EVIOCGABS(axis_val), &axis_info) < 0) {
    raise_for_ioctl_error(fd, "EVIOCSREP", EVIOCSREP);
  }
  VALUE results_hash = rb_hash_new();
  rb_hash_aset(results_hash, rb_str_new2("value"), INT2NUM(axis_info.value));
  rb_hash_aset(results_hash, rb_str_new2("minimum"), INT2NUM(axis_info.minimum));
  rb_hash_aset(results_hash, rb_str_new2("maximum"), INT2NUM(axis_info.maximum));
  rb_hash_aset(results_hash, rb_str_new2("fuzz"), INT2NUM(axis_info.fuzz));
  rb_hash_aset(results_hash, rb_str_new2("flat"), INT2NUM(axis_info.flat));
  return results_hash;
}

/* call-seq: 
 *   scancode_for_keycode(int_keycode) -> aNumber
 *
 * For key input devices with drivers that support it, this function returns the
 * scancode currently mapped to the input keycode.  Not all keyboard drivers
 * support this (the USB drivers, in particular, do not).  If the operation is
 * unsupported, this function with raise and IOException.
 */
static VALUE ed_scancode_for_keycode(VALUE self, VALUE keycode)
{
  int fd = get_fd(self);
  int mapping[2];
  mapping[0] = NUM2INT(keycode);

  if (ioctl(fd, EVIOCGKEYCODE, mapping) < 0) {
    raise_for_ioctl_error(fd, "EVIOCGKEYCODE", EVIOCGKEYCODE);
  }
  return INT2NUM(mapping[1]);
}

/* call-seq:
 *     set_scancode_for_keycode(int_keycode, int_new_scancode) -> nil
 *
 * For key input devices with drivers that support it, this function remaps the
 * scancode resulting from a keypress.  Not all keyboard drivers support this
 * (the USB drivers, in particular, do not).  If the operation is unsupported,
 * this function with raise and IOException.
 */
static VALUE ed_set_scancode_for_keycode(VALUE self, VALUE keycode, VALUE scancode)
{
  int fd = get_fd(self);
  int mapping[2];
  mapping[0] = NUM2INT(keycode);
  mapping[1] = NUM2INT(scancode);

  if (ioctl(fd, EVIOCSKEYCODE, mapping) < 0) {
    raise_for_ioctl_error(fd, "EVIOCGKEYCODE", EVIOCGKEYCODE);
  }
  return Qnil;
}

void Init_eventdevice_c() {
  VALUE module = rb_define_module("Evdev");
  cEventDevice = rb_define_class_under(module, "EventDevice", rb_cFile);
  rb_define_method(cEventDevice, "event_interface_version", ed_event_interface_version, 0);
  rb_define_method(cEventDevice, "bus_type_code", ed_bus_type_code, 0);
  rb_define_method(cEventDevice, "vendor", ed_vendor, 0);
  rb_define_method(cEventDevice, "product", ed_product, 0);
  rb_define_method(cEventDevice, "version", ed_version, 0);
  rb_define_method(cEventDevice, "device_name", ed_device_name, 0);
  rb_define_method(cEventDevice, "topology", ed_topology, 0);
  rb_define_method(cEventDevice, "unique_id", ed_unique_id, 0);
  rb_define_method(cEventDevice, "supported_feature_types", ed_supported_feature_types, 0);
  rb_define_method(cEventDevice, "supported_features_for_type", ed_supported_features_for_type, 1);
  rb_define_method(cEventDevice, "activated_keys", ed_activated_keys, 0);
  rb_define_method(cEventDevice, "activated_leds", ed_activated_leds, 0);
  rb_define_method(cEventDevice, "activated_sounds", ed_activated_sounds, 0);
  rb_define_method(cEventDevice, "read_event", ed_read_event, 0); 
  rb_define_method(cEventDevice, "write_event", ed_write_event, 3);
  rb_define_method(cEventDevice, "repeat_rate", ed_repeat_rate, 0);
  rb_define_method(cEventDevice, "set_repeat_rate", ed_set_repeat_rate, 2);
  rb_define_method(cEventDevice, "parameters_for_axis", ed_parameters_for_axis, 1);
  rb_define_method(cEventDevice, "scancode_for_keycode", ed_scancode_for_keycode, 1);
  rb_define_method(cEventDevice, "set_scancode_for_keycode", ed_set_scancode_for_keycode, 2); 
}
