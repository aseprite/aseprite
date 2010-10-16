/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      MacOS X HID Manager access routines.
 *
 *      By Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintosx.h"

#ifndef ALLEGRO_MACOSX
#error Something is wrong with the makefile
#endif

#define USAGE(p, u) (((p)<<16)+(u))

static void hid_store_element_data(CFTypeRef element, int type, HID_DEVICE *device, int app, int collection, int index);
static void hid_scan_physical_collection(CFTypeRef properties, HID_DEVICE *device, int app, int collection);
static void hid_scan_application_collection(CFMutableDictionaryRef properties, HID_DEVICE *device);

static HID_DEVICE* add_device(HID_DEVICE_COLLECTION*);

/* add_element:
 * Add a new, blank element to a device.
 * This is later specialized into a button
 * axis or whatever 
 */
static HID_ELEMENT* add_element(HID_DEVICE* d) {
   HID_ELEMENT* e;
   if (d->element==NULL) {
      d->capacity=8;
      d->element=malloc(d->capacity*sizeof(HID_ELEMENT));
   }
   if (d->num_elements>=d->capacity) {
      d->capacity*=2;
      d->element=realloc(d->element, d->capacity*sizeof(HID_ELEMENT));
   }
   e=&d->element[d->num_elements++];
   memset(e, 0, sizeof(HID_ELEMENT));
   return e;
}

/* i_val:
 * Helper method - get an integer value from a dictionary
 * Defaults to 0 if the value is not found; in practice this 
 * should not occur.
 */
static int i_val(CFTypeRef d, CFStringRef key) {
   int ans;
   CFTypeRef num = CFDictionaryGetValue(d, key);
   if (num) 
      CFNumberGetValue(num, kCFNumberIntType, &ans);
   else
      ans = 0;
   return ans;
}

/* hid_store_element_data:
 * Parse this HID element, break it down and store the 
 * relevant data in the device structure
 */
static void hid_store_element_data(CFTypeRef element, int type, HID_DEVICE *device, int app, int col, int idx)
{
   HID_ELEMENT *hid_element;
   CFTypeRef type_ref;
   AL_CONST char *name;

   hid_element = add_element(device);
   hid_element->type = type;
   hid_element->index = idx;
   hid_element->col = col;
   hid_element->app = app;
   hid_element->cookie = (IOHIDElementCookie) i_val(element, CFSTR(kIOHIDElementCookieKey));
   hid_element->min = i_val(element, CFSTR(kIOHIDElementMinKey));
   hid_element->max = i_val(element, CFSTR(kIOHIDElementMaxKey));
   type_ref = CFDictionaryGetValue(element, CFSTR(kIOHIDElementNameKey));
   if ((type_ref) && (name = CFStringGetCStringPtr(type_ref, CFStringGetSystemEncoding())))
      hid_element->name = strdup(name);
   else
      hid_element->name = NULL;
}

/* hid_scan_application:
 * scan the elements that make up one 'application'
 * i.e. one unit like a joystick.
 */
static void hid_scan_application(CFTypeRef properties, HID_DEVICE *device, int app)
{
   CFTypeRef array_ref, element;
   int type, usage_page, usage;
   int i;
   int axis=0;
   int stick=0;
   array_ref = CFDictionaryGetValue(properties, CFSTR(kIOHIDElementKey));
   if ((array_ref) && (CFGetTypeID(array_ref) == CFArrayGetTypeID())) {
      for (i = 0; i < CFArrayGetCount(array_ref); i++) {
         element = CFArrayGetValueAtIndex(array_ref, i);
         if (CFGetTypeID(element) == CFDictionaryGetTypeID()) {
            type = i_val(element, CFSTR(kIOHIDElementTypeKey));
            usage = i_val(element, CFSTR(kIOHIDElementUsageKey));
            usage_page = i_val(element, CFSTR(kIOHIDElementUsagePageKey));
            if (type == kIOHIDElementTypeCollection) 
               {
                  /* It is a collection; recurse into it, if it is
                   part of the generic desktop (sometimes the whole
                   joystick is wrapped up inside a collection like
                   this. */
                  if (usage_page == kHIDPage_GenericDesktop)
                     hid_scan_application(element, device, app);                     
               }
            else
               { 
                  switch (usage_page) {
                  case kHIDPage_GenericDesktop:
                     switch(usage) {
                     case kHIDUsage_GD_Pointer:
                        if (axis!=0) {
                           /* already have some elements in this stick */
                           ++stick;
                           axis=0;
                        }
                        hid_scan_physical_collection(element, device, app, stick);
                        ++stick;
                        break;
                     case kHIDUsage_GD_X:
                        hid_store_element_data(element, HID_ELEMENT_AXIS_PRIMARY_X, device, app, stick, axis++);
                        break;
                        
                     case kHIDUsage_GD_Y:
                        hid_store_element_data(element, HID_ELEMENT_AXIS_PRIMARY_Y, device, app, stick, axis++);
                        break;
                        
                     case kHIDUsage_GD_Z:
                     case kHIDUsage_GD_Rx:
                     case kHIDUsage_GD_Ry:
                     case kHIDUsage_GD_Rz:
                        hid_store_element_data(element, HID_ELEMENT_AXIS, device,app, stick, axis++);
                        break;
                        
                     case kHIDUsage_GD_Slider:
                     case kHIDUsage_GD_Dial:
                     case kHIDUsage_GD_Wheel:
                        /* If we've already seen some axes on this stick, move to the next one */
                        if (axis > 0) 
                           {
                              ++stick;
                              axis=0;
                           }
                        hid_store_element_data(element, HID_ELEMENT_STANDALONE_AXIS, device, app, stick++, 0);
                        break;
                    
                     case kHIDUsage_GD_Hatswitch:
                        /* If we've already seen some axes on this stick, move to the next one */
                        if (axis > 0) 
                           {
                              ++stick;
                              axis=0;
                           }
                        hid_store_element_data(element, HID_ELEMENT_HAT, device, app, stick++, 0);
                        break;
                     }
                     break;
                  case kHIDPage_Button:
                     hid_store_element_data(element, HID_ELEMENT_BUTTON, device, app, 0, usage-1);
                     break;
                  }
               }
            if (axis>=2) {
               ++stick;
               axis=0;
            }
         }
      }
   }
}

/* hid_scan_physical_collection:
 * scan the elements that make up one 'stick'
 */
static void hid_scan_physical_collection(CFTypeRef properties, HID_DEVICE *device, int app, int stick)
{
   CFTypeRef array_ref, element;
   int type, usage_page, usage;
   int i;
   int axis=0;
   array_ref = CFDictionaryGetValue(properties, CFSTR(kIOHIDElementKey));
   if ((array_ref) && (CFGetTypeID(array_ref) == CFArrayGetTypeID())) {
      for (i = 0; i < CFArrayGetCount(array_ref); i++) {
         element = CFArrayGetValueAtIndex(array_ref, i);
         if (CFGetTypeID(element) == CFDictionaryGetTypeID()) {
            type = i_val(element, CFSTR(kIOHIDElementTypeKey));
            usage = i_val(element, CFSTR(kIOHIDElementUsageKey));
            usage_page = i_val(element, CFSTR(kIOHIDElementUsagePageKey));
            switch (usage_page) {
            case kHIDPage_GenericDesktop:
               switch(usage) {
               case kHIDUsage_GD_X:
                  hid_store_element_data(element, HID_ELEMENT_AXIS_PRIMARY_X, device, app, stick, axis++);
                  break;
                
               case kHIDUsage_GD_Y:
                  hid_store_element_data(element, HID_ELEMENT_AXIS_PRIMARY_Y, device, app, stick, axis++);
                  break;
                
               case kHIDUsage_GD_Z:
               case kHIDUsage_GD_Rx:
               case kHIDUsage_GD_Ry:
               case kHIDUsage_GD_Rz:
                  hid_store_element_data(element, HID_ELEMENT_AXIS, device,app, stick, axis++);
                  break;
               }
               break;
            case kHIDPage_Button:
               hid_store_element_data(element, HID_ELEMENT_BUTTON, device, app, 0, usage-1);
               break;
            }
         }
      }
   }
}

/* hid_scan_application_collection:
 * Scan the elements array; each element will be an 'application'
 * i.e. one joystick, gamepad or mouse
 *
 */
static void hid_scan_application_collection(CFMutableDictionaryRef properties, HID_DEVICE *device)
{
   CFTypeRef array_ref, element;

   int usage, usage_page;
   int i;

   array_ref = CFDictionaryGetValue(properties, CFSTR(kIOHIDElementKey));
   if ((array_ref) && (CFGetTypeID(array_ref) == CFArrayGetTypeID())) {
      for (i = 0; i < CFArrayGetCount(array_ref); i++) {
         element = CFArrayGetValueAtIndex(array_ref, i);
         if (CFGetTypeID(element) == CFDictionaryGetTypeID()) {
            usage=i_val(element, CFSTR(kIOHIDElementUsageKey));
            usage_page=i_val(element, CFSTR(kIOHIDElementUsagePageKey));
            switch(USAGE(usage_page,  usage)) {
            case USAGE(kHIDPage_GenericDesktop, kHIDUsage_GD_Joystick):
               device->type=HID_JOYSTICK;
               hid_scan_application(element, device, device->cur_app);
               device->cur_app++;
               break;
            case USAGE(kHIDPage_GenericDesktop, kHIDUsage_GD_GamePad):
               device->type=HID_GAMEPAD;
               hid_scan_application(element, device, device->cur_app);
               device->cur_app++;
               break;
            case USAGE(kHIDPage_GenericDesktop, kHIDUsage_GD_Mouse):
               device->type=HID_MOUSE;
               hid_scan_application(element, device, device->cur_app);
               device->cur_app++;
               break;
            }
         }
      }
   }
}

/* get_usb_properties:
 * Get a property dictionary from the USB plane. 
 */
static CFMutableDictionaryRef get_usb_properties(io_object_t device)
{
   io_registry_entry_t parent, grandparent;
   CFMutableDictionaryRef usb_properties = NULL;
   if (IORegistryEntryGetParentEntry(device, kIOServicePlane, &parent) == KERN_SUCCESS) {
      if (IORegistryEntryGetParentEntry(parent, kIOServicePlane, &grandparent) == KERN_SUCCESS) {
         IORegistryEntryCreateCFProperties (grandparent, &usb_properties, 
                                            kCFAllocatorDefault, kNilOptions);
         IOObjectRelease(grandparent);
      }
      IOObjectRelease(parent);
   }
   return usb_properties;
}

#if OSX_HID_PSEUDO_SCAN
/*
 * Pseudo scan - for development purposes, if someone has hardware
 * that isn't parsed by this code, you can ask them to dump _their_ scan
 * as a plist, then reload it here in order to debug it.
*/
HID_DEVICE_COLLECTION *osx_hid_scan(int type, HID_DEVICE_COLLECTION* col)
{
   HID_DEVICE* this_device;
   NSDictionary* properties = nil;
   NSString* filename;
   switch (type) {
   case HID_GAMEPAD:
      filename = @"gamepad.plist";
      break;
   case HID_JOYSTICK:
      filename = @"joystick.plist";
      break;
   case HID_MOUSE:
      filename = @"mouse.plist";
      break;
   default:
      filename = nil;
      break;
   }
   if (filename != nil)
      properties = [NSDictionary dictionaryWithContentsOfFile:filename];
   if (properties)
   {   
      this_device = add_device(col);
      this_device->manufacturer = strdup([((NSString*) [properties objectForKey: @kIOHIDManufacturerKey]) lossyCString]);
      this_device->product = strdup([((NSString*) [properties objectForKey: @kIOHIDProductKey]) lossyCString]);
      hid_scan_application_collection((CFDictionaryRef) properties, this_device);
      [properties release];
   }
   return col;
}
#else
/* osx_hid_scan:
 * Scan the hid manager for devices of type 'type',
 * and append to the collection col
 */
HID_DEVICE_COLLECTION *osx_hid_scan(int type, HID_DEVICE_COLLECTION* col)
{
   ASSERT(col);
   HID_DEVICE *this_device;
   mach_port_t master_port = 0;
   io_iterator_t hid_object_iterator = 0;
   io_object_t hid_device = 0;
   CFMutableDictionaryRef class_dictionary = NULL;
   int usage, usage_page;
   CFTypeRef type_ref;
   CFNumberRef usage_ref = NULL, usage_page_ref = NULL;
   CFMutableDictionaryRef properties = NULL, usb_properties = NULL;
   IOCFPlugInInterface **plugin_interface = NULL;
   IOReturn result;
   AL_CONST char *string;
   SInt32 score = 0;
   int error;
   
   usage_page = kHIDPage_GenericDesktop;
   switch (type) {
   case HID_MOUSE:
      usage = kHIDUsage_GD_Mouse;
      break;
   case HID_JOYSTICK:
      usage = kHIDUsage_GD_Joystick;
      break;
   case HID_GAMEPAD:
      usage=kHIDUsage_GD_GamePad;
      break;
   }

   result = IOMasterPort(bootstrap_port, &master_port);
   if (result == kIOReturnSuccess) {
      class_dictionary = IOServiceMatching(kIOHIDDeviceKey);
      if (class_dictionary) {
         /* Add key for device type to refine the matching dictionary. */
         usage_ref = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usage);
         usage_page_ref = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &usage_page);
         CFDictionarySetValue(class_dictionary, CFSTR(kIOHIDPrimaryUsageKey), usage_ref);
         CFDictionarySetValue(class_dictionary, CFSTR(kIOHIDPrimaryUsagePageKey), usage_page_ref);
      }
      result = IOServiceGetMatchingServices(master_port, class_dictionary, &hid_object_iterator);
      if ((result == kIOReturnSuccess) && (hid_object_iterator)) {
         /* Ok, we have a list of attached HID devices; scan them. */
         while ((hid_device = IOIteratorNext(hid_object_iterator))!=0) {
            if ((IORegistryEntryCreateCFProperties(hid_device, &properties, kCFAllocatorDefault, kNilOptions) == KERN_SUCCESS) && (properties != NULL)) {
               error = FALSE;
               this_device = add_device(col);
               this_device->type = type;
               if (col->count==0) {
                  this_device->cur_app=0;
               }
               else {
                  this_device->cur_app=col->devices[col->count-1].cur_app;
               }
               /* 
                * Mac OS X currently is not mirroring all USB properties
                * to HID page so need to look at USB device page also. 
                */
               usb_properties = get_usb_properties(hid_device);
               type_ref = CFDictionaryGetValue(properties, CFSTR(kIOHIDManufacturerKey));
               if (!type_ref)
                  type_ref = CFDictionaryGetValue(usb_properties, CFSTR("USB Vendor Name"));
               if ((type_ref) && (string = CFStringGetCStringPtr(type_ref, CFStringGetSystemEncoding())))
                  this_device->manufacturer = strdup(string);
               else
                  this_device->manufacturer = NULL;
               type_ref = CFDictionaryGetValue(properties, CFSTR(kIOHIDProductKey));
               if (!type_ref)
                  type_ref = CFDictionaryGetValue(usb_properties, CFSTR("USB Product Name"));
               if ((type_ref) && (string = CFStringGetCStringPtr(type_ref, CFStringGetSystemEncoding())))
                  this_device->product = strdup(string);
               else
                  this_device->product = NULL;

               type_ref = CFDictionaryGetValue(usb_properties, CFSTR("USB Address"));
               if ((type == HID_MOUSE) && (!type_ref)) {
                  /* Not an USB mouse. Must be integrated trackpad: we report it as a single button mouse */
                  add_element(this_device)->type = HID_ELEMENT_BUTTON;
               }
               else {
                  /* Scan for device elements */
                  this_device->num_elements = 0;
                  hid_scan_application_collection(properties, this_device);
               }

               this_device->interface = NULL;
               if ((type == HID_JOYSTICK) || (type == HID_GAMEPAD)) {
                  /* Joystick or gamepad device: create HID interface */
                  if (IOCreatePlugInInterfaceForService(hid_device, kIOHIDDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &plugin_interface, &score) != kIOReturnSuccess)
                     error = TRUE;
                  else {
                     if ((*plugin_interface)->QueryInterface(plugin_interface, CFUUIDGetUUIDBytes(kIOHIDDeviceInterfaceID), (void *)&(this_device->interface)) != S_OK)
                        error = TRUE;
                     (*plugin_interface)->Release(plugin_interface);
                     if ((*(this_device->interface))->open(this_device->interface, 0) != KERN_SUCCESS)
                        error = TRUE;
                  }
               }
               if (error) {
                  if (this_device->manufacturer)
                     free(this_device->manufacturer);
                  if (this_device->product)
                     free(this_device->product);
                  if (this_device->interface)
                     (*(this_device->interface))->Release(this_device->interface);
                  this_device->interface=NULL;
                  --col->count;
               }

               CFRelease(properties);
               CFRelease(usb_properties);
            }
         }
         IOObjectRelease(hid_object_iterator);
      }
      CFRelease(usage_ref);
      CFRelease(usage_page_ref);
      mach_port_deallocate(mach_task_self(), master_port);
   }

   return col;
}
#endif
/* add_device:
 * add a new device to a collection
 */
static HID_DEVICE* add_device(HID_DEVICE_COLLECTION* o) {
   HID_DEVICE* d;
   if (o->devices==NULL) {
      o->count=0;
      o->capacity=1;
      o->devices=malloc(o->capacity*sizeof(HID_DEVICE));
   }
   if (o->count>=o->capacity) {
      o->capacity*=2;
      o->devices=realloc(o->devices, o->capacity*sizeof(HID_DEVICE));
   }
   d=&o->devices[o->count];
   memset(d, 0, sizeof(HID_DEVICE));
   /* Chain onto the preceding app count */
   if (o->count>0)
      d->cur_app = o->devices[o->count-1].cur_app;
   ++o->count;
   return d;
}

/* osx_hid_free:
 * Release the memory taken up by a collection 
 */
void osx_hid_free(HID_DEVICE_COLLECTION *col)
{
   HID_DEVICE *device;
   HID_ELEMENT *element;
   int i, j;

   for (i = 0; i < col->count; i++) {
      device = &col->devices[i];
      if (device->manufacturer)
         free(device->manufacturer);
      if (device->product)
         free(device->product);
      for (j = 0; j < device->num_elements; j++) {
         element = &device->element[j];
         if (element->name)
            free(element->name);
      }
      free(device->element);
      if (device->interface) {
         (*(device->interface))->close(device->interface);
         (*(device->interface))->Release(device->interface);
      }
   }
   free(col->devices);
}

/* Local variables:       */
/* c-basic-offset: 3      */
/* indent-tabs-mode: nil  */
/* End:                   */
