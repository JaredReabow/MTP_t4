// MTP.h - Teensy MTP Responder library
// Copyright (C) 2017 Fredrik Hubinette <hubbe@hubbe.net>
//
// With updates from MichaelMC and Yoong Hor Meng <yoonghm@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// modified for SDFS by WMXZ
// modified for T4 by WMXZ

#include "core_pins.h"
//#include "usb_dev.h"
//#include "usb_serial.h"
#include "MTP.h"

#if defined(__IMXRT1062__)
  #include "debug/printf.h"
#else
  #define printf_init()
  #define printf(...)
  #define printf_debug_init()
#endif

extern "C"  uint32_t mtp_tx_counter;
extern "C"  uint32_t mtp_rx_counter;
extern "C"  uint32_t mtp_rx_event_counter;
extern "C"  uint32_t mtp_tx_event_counter;

extern volatile uint8_t usb_high_speed;

// These should probably be weak.
extern void mtp_yield();
extern void mtp_lock_storage(bool lock);

// Container Types
#define MTP_CONTAINER_TYPE_UNDEFINED    0
#define MTP_CONTAINER_TYPE_COMMAND      1
#define MTP_CONTAINER_TYPE_DATA         2
#define MTP_CONTAINER_TYPE_RESPONSE     3
#define MTP_CONTAINER_TYPE_EVENT        4

// Container Offsets
#define MTP_CONTAINER_LENGTH_OFFSET             0
#define MTP_CONTAINER_TYPE_OFFSET               4
#define MTP_CONTAINER_CODE_OFFSET               6
#define MTP_CONTAINER_TRANSACTION_ID_OFFSET     8
#define MTP_CONTAINER_PARAMETER_OFFSET          12
#define MTP_CONTAINER_HEADER_SIZE               12

// MTP Operation Codes
#define MTP_OPERATION_GET_DEVICE_INFO                       0x1001
#define MTP_OPERATION_OPEN_SESSION                          0x1002
#define MTP_OPERATION_CLOSE_SESSION                         0x1003
#define MTP_OPERATION_GET_STORAGE_IDS                       0x1004
#define MTP_OPERATION_GET_STORAGE_INFO                      0x1005
#define MTP_OPERATION_GET_NUM_OBJECTS                       0x1006
#define MTP_OPERATION_GET_OBJECT_HANDLES                    0x1007
#define MTP_OPERATION_GET_OBJECT_INFO                       0x1008
#define MTP_OPERATION_GET_OBJECT                            0x1009
#define MTP_OPERATION_GET_THUMB                             0x100A
#define MTP_OPERATION_DELETE_OBJECT                         0x100B
#define MTP_OPERATION_SEND_OBJECT_INFO                      0x100C
#define MTP_OPERATION_SEND_OBJECT                           0x100D
#define MTP_OPERATION_INITIATE_CAPTURE                      0x100E
#define MTP_OPERATION_FORMAT_STORE                          0x100F
#define MTP_OPERATION_RESET_DEVICE                          0x1010
#define MTP_OPERATION_SELF_TEST                             0x1011
#define MTP_OPERATION_SET_OBJECT_PROTECTION                 0x1012
#define MTP_OPERATION_POWER_DOWN                            0x1013
#define MTP_OPERATION_GET_DEVICE_PROP_DESC                  0x1014
#define MTP_OPERATION_GET_DEVICE_PROP_VALUE                 0x1015
#define MTP_OPERATION_SET_DEVICE_PROP_VALUE                 0x1016
#define MTP_OPERATION_RESET_DEVICE_PROP_VALUE               0x1017
#define MTP_OPERATION_TERMINATE_OPEN_CAPTURE                0x1018
#define MTP_OPERATION_MOVE_OBJECT                           0x1019
#define MTP_OPERATION_COPY_OBJECT                           0x101A
#define MTP_OPERATION_GET_PARTIAL_OBJECT                    0x101B
#define MTP_OPERATION_INITIATE_OPEN_CAPTURE                 0x101C
#define MTP_OPERATION_GET_OBJECT_PROPS_SUPPORTED            0x9801
#define MTP_OPERATION_GET_OBJECT_PROP_DESC                  0x9802
#define MTP_OPERATION_GET_OBJECT_PROP_VALUE                 0x9803
#define MTP_OPERATION_SET_OBJECT_PROP_VALUE                 0x9804
#define MTP_OPERATION_GET_OBJECT_PROP_LIST                  0x9805
#define MTP_OPERATION_SET_OBJECT_PROP_LIST                  0x9806
#define MTP_OPERATION_GET_INTERDEPENDENT_PROP_DESC          0x9807
#define MTP_OPERATION_SEND_OBJECT_PROP_LIST                 0x9808
#define MTP_OPERATION_GET_OBJECT_REFERENCES                 0x9810
#define MTP_OPERATION_SET_OBJECT_REFERENCES                 0x9811
#define MTP_OPERATION_SKIP                                  0x9820


const unsigned short supported_op[]=
{
	MTP_OPERATION_GET_DEVICE_INFO                        ,//0x1001
	MTP_OPERATION_OPEN_SESSION                           ,//0x1002
	MTP_OPERATION_CLOSE_SESSION                          ,//0x1003
	MTP_OPERATION_GET_STORAGE_IDS                        ,//0x1004
	MTP_OPERATION_GET_STORAGE_INFO                       ,//0x1005
	//MTP_OPERATION_GET_NUM_OBJECTS                        ,//0x1006
	MTP_OPERATION_GET_OBJECT_HANDLES                     ,//0x1007
	MTP_OPERATION_GET_OBJECT_INFO                        ,//0x1008
	MTP_OPERATION_GET_OBJECT                             ,//0x1009
	//MTP_OPERATION_GET_THUMB                              ,//0x100A
	MTP_OPERATION_DELETE_OBJECT                          ,//0x100B
	MTP_OPERATION_SEND_OBJECT_INFO                       ,//0x100C
	MTP_OPERATION_SEND_OBJECT                            ,//0x100D
	MTP_OPERATION_GET_DEVICE_PROP_DESC                   ,//0x1014
	MTP_OPERATION_GET_DEVICE_PROP_VALUE                  ,//0x1015
	//MTP_OPERATION_SET_DEVICE_PROP_VALUE                  ,//0x1016
	//MTP_OPERATION_RESET_DEVICE_PROP_VALUE                ,//0x1017
  MTP_OPERATION_MOVE_OBJECT                           ,//0x1019
  //MTP_OPERATION_COPY_OBJECT                           0x101A

	//MTP_OPERATION_GET_PARTIAL_OBJECT                     ,//0x101B
	MTP_OPERATION_GET_OBJECT_PROPS_SUPPORTED             ,//0x9801
	MTP_OPERATION_GET_OBJECT_PROP_DESC                   ,//0x9802
	MTP_OPERATION_GET_OBJECT_PROP_VALUE                  ,//0x9803
	MTP_OPERATION_SET_OBJECT_PROP_VALUE                  //0x9804
	//MTP_OPERATION_GET_OBJECT_PROP_LIST                   ,//0x9805
	//MTP_OPERATION_GET_OBJECT_REFERENCES                  ,//0x9810
	//MTP_OPERATION_SET_OBJECT_REFERENCES                  ,//0x9811
	//MTP_OPERATION_GET_PARTIAL_OBJECT_64                  ,//0x95C1
	//MTP_OPERATION_SEND_PARTIAL_OBJECT                    ,//0x95C2
	//MTP_OPERATION_TRUNCATE_OBJECT                        ,//0x95C3
  //MTP_OPERATION_BEGIN_EDIT_OBJECT                      ,//0x95C4
	//MTP_OPERATION_END_EDIT_OBJECT                         //0x95C5
};
const int supported_op_size=sizeof(supported_op);
const int supported_op_num = supported_op_size/sizeof(supported_op[0]);

  void MTPD::WriteDescriptor() {
    write16(100);  // MTP version
    write32(6);    // MTP extension
//    write32(0xFFFFFFFFUL);    // MTP extension
    write16(100);  // MTP version
    writestring("microsoft.com: 1.0;");
    write16(0);    // functional mode

    // Supported operations (array of uint16)
    write32(supported_op_num);
    for(int ii=0; ii<supported_op_num;ii++) write16(supported_op[ii]);

    write32(0);       // Events (array of uint16)
    //write16(0x4008);    // Device Info changed // cannot get this working!

    write32(1);       // Device properties (array of uint16)
    write16(0xd402);  // Device friendly name

    write32(0);       // Capture formats (array of uint16)

    write32(2);       // Playback formats (array of uint16)
    write16(0x3000);  // Undefined format
    write16(0x3001);  // Folders (associations)

    writestring("PJRC");     // Manufacturer
    writestring("Teensy");   // Model
    writestring("1.0");      // version
    writestring("???");      // serial
  }

  void MTPD::GetDevicePropValue(uint32_t prop) {
    switch (prop) {
      case 0xd402: // friendly name
        // This is the name we'll actually see in the windows explorer.
        // Should probably be configurable.
        writestring("Teensy");
        break;
    }
  }

  void MTPD::GetDevicePropDesc(uint32_t prop) {
    switch (prop) {
      case 0xd402: // friendly name
        write16(prop);
        write16(0xFFFF); // string type
        write8(0);       // read-only
        GetDevicePropValue(prop);
        GetDevicePropValue(prop);
        write8(0);       // no form
    }
  }

  void MTPD::WriteStorageIDs() {
    write32(1); // 1 entry
    write32(1); // 1 storage
  }

  void MTPD::GetStorageInfo(uint32_t storage) {
    uint16_t readOnly = storage_->readonly();
    uint16_t has_directories = storage_->has_directories();
    uint64_t storage_size = storage_->size();
    uint64_t storage_free = storage_->free();
    
    write16(readOnly ? 0x0001 : 0x0004);   // storage type (removable RAM)
    write16(has_directories ? 0x0002: 0x0001);   // filesystem type (generic hierarchical)
    write16(0x0000);   // access capability (read-write)
    write64(storage_size);  // max capacity
    write64(storage_free);  // free space (100M)
    write32(0xFFFFFFFFUL);  // free space (objects)
    writestring("SD Card");  // storage descriptor
    writestring("");  // volume identifier
  }

  uint32_t MTPD::GetNumObjects(uint32_t storage, uint32_t parent) 
  {
    storage_->StartGetObjectHandles(parent);
    int num = 0;
    while (storage_->GetNextObjectHandle()) num++;
    return num;
  }

  void MTPD::GetObjectHandles(uint32_t storage, uint32_t parent) 
  {
    uint32_t num = 0;
    if (!write_get_length_) {
      num = GetNumObjects(storage, parent);
    }
    write32(num);
    int handle;
    storage_->StartGetObjectHandles(parent);
    while ((handle = storage_->GetNextObjectHandle()))
      write32(handle);
  }

  void MTPD::GetObjectInfo(uint32_t handle) 
  {
    char filename[256];
    uint32_t dir, size, parent;
    storage_->GetObjectInfo(handle, filename, &dir, &size, &parent);

    write32(1); // storage
    write16(dir? 0x3001 : 0x3000); // format
    write16(0);  // protection
    write32(size); // size
    write16(0); // thumb format
    write32(0); // thumb size
    write32(0); // thumb width
    write32(0); // thumb height
    write32(0); // pix width
    write32(0); // pix height
    write32(0); // bit depth
    write32(parent); // parent
    //write16(size == 0xFFFFFFFFUL ? 1 : 0); // association type
    write16(dir);
    write32(0); // association description
    write32(0);  // sequence number
    writestring(filename);
    writestring("");  // date created
    writestring("");  // date modified
    writestring("");  // keywords
  }


  void MTPD::write8 (uint8_t  x) { write((char*)&x, sizeof(x)); }
  void MTPD::write16(uint16_t x) { write((char*)&x, sizeof(x)); }
  void MTPD::write32(uint32_t x) { write((char*)&x, sizeof(x)); }
  void MTPD::write64(uint64_t x) { write((char*)&x, sizeof(x)); }

  void MTPD::writestring(const char* str) {
    if (*str) 
    { write8(strlen(str) + 1);
      while (*str) {  write16(*str);  ++str;  } write16(0);
    } else 
    { write8(0);
    }
  }

  uint32_t MTPD::ReadMTPHeader() {
    MTPHeader header;
    read(0,0);
    read((char *)&header, sizeof(MTPHeader));
    // check that the type is data
    return header.len - 12;
  }

  uint8_t MTPD::read8() {
    uint8_t ret;
    read((char*)&ret, sizeof(ret));
    return ret;
  }

  uint16_t MTPD::read16() {
    uint16_t ret;
    read((char*)&ret, sizeof(ret));
    return ret;
  }

  uint32_t MTPD::read32() {
    uint32_t ret;
    read((char*)&ret, sizeof(ret));
    return ret;
  }

  uint32_t MTPD::readstring(char* buffer) {
    int len = read8();
    if (!buffer) {
      read(NULL, len * 2);
    } else {
      for (int i = 0; i < len; i++) {
        *(buffer++) = read16();
      }
    }
    *(buffer++)=0; *(buffer++)=0; // to be sure
    return 2*len+1;
  }


#define MTP_PROPERTY_STORAGE_ID                             0xDC01
#define MTP_PROPERTY_OBJECT_FORMAT                          0xDC02
#define MTP_PROPERTY_PROTECTION_STATUS                      0xDC03
#define MTP_PROPERTY_OBJECT_SIZE                            0xDC04
#define MTP_PROPERTY_OBJECT_FILE_NAME                       0xDC07
#define MTP_PROPERTY_DATE_CREATED                           0xDC08
#define MTP_PROPERTY_DATE_MODIFIED                          0xDC09
#define MTP_PROPERTY_PARENT_OBJECT                          0xDC0B
#define MTP_PROPERTY_PERSISTENT_UID                         0xDC41
#define MTP_PROPERTY_NAME                                   0xDC44

const uint16_t propertyList[] =
{
  MTP_PROPERTY_STORAGE_ID                             ,//0xDC01
  MTP_PROPERTY_OBJECT_FORMAT                          ,//0xDC02
  MTP_PROPERTY_PROTECTION_STATUS                      ,//0xDC03
  MTP_PROPERTY_OBJECT_SIZE                            ,//0xDC04
  MTP_PROPERTY_OBJECT_FILE_NAME                       ,//0xDC07
  MTP_PROPERTY_DATE_CREATED                           ,//0xDC08
  MTP_PROPERTY_DATE_MODIFIED                          ,//0xDC09
  MTP_PROPERTY_PARENT_OBJECT                          ,//0xDC0B
  MTP_PROPERTY_PERSISTENT_UID                         ,//0xDC41
  MTP_PROPERTY_NAME                                    //0xDC44
};

uint32_t propertyListNum = sizeof(propertyList)/sizeof(propertyList[0]);

  void MTPD::getObjectPropsSupported(uint32_t p1)
  {
    write32(propertyListNum);
    for(uint32_t ii=0; ii<propertyListNum;ii++) write16(propertyList[ii]);
  }

  void MTPD::getObjectPropDesc(uint32_t p1, uint32_t p2)
  {
    switch(p1)
    {
      case MTP_PROPERTY_STORAGE_ID:         //0xDC01:
        write16(0xDC01);
        write16(0x006);
        write8(0); //get
        write32(0);
        write32(0);
        write8(0);
        break;
      case MTP_PROPERTY_OBJECT_FORMAT:        //0xDC02:
        write16(0xDC02);
        write16(0x004);
        write8(0); //get
        write16(0);
        write32(0);
        write8(0);
        break;
      case MTP_PROPERTY_PROTECTION_STATUS:    //0xDC03:
        write16(0xDC03);
        write16(0x004);
        write8(0); //get
        write16(0);
        write32(0);
        write8(0);
        break;
      case MTP_PROPERTY_OBJECT_SIZE:        //0xDC04:
        write16(0xDC04);
        write16(0x008);
        write8(0); //get
        write64(0);
        write32(0);
        write8(0);
        break;
      case MTP_PROPERTY_OBJECT_FILE_NAME:   //0xDC07:
        write16(0xDC07);
        write16(0xFFFF);
        write8(1); //get/set
        write8(0);
        write32(0);
        write8(0);
        break;
      case MTP_PROPERTY_DATE_CREATED:       //0xDC08:
        write16(0xDC08);
        write16(0xFFFF);
        write8(0); //get
        write8(0);
        write32(0);
        write8(0);
        break;
      case MTP_PROPERTY_DATE_MODIFIED:      //0xDC09:
        write16(0xDC09);
        write16(0xFFFF);
        write8(0); //get
        write8(0);
        write32(0);
        write8(0);
        break;
      case MTP_PROPERTY_PARENT_OBJECT:    //0xDC0B:
        write16(0xDC0B);
        write16(6);
        write8(0); //get
        write32(0);
        write32(0);
        write8(0);
        break;
      case MTP_PROPERTY_PERSISTENT_UID:   //0xDC41:
        write16(0xDC41);
        write16(0x0A);
        write8(0); //get
        write64(0);
        write64(0);
        write32(0);
        write8(0);
        break;
      case MTP_PROPERTY_NAME:             //0xDC44:
        write16(0xDC44);
        write16(0xFFFF);
        write8(0); //get
        write8(0);
        write32(0);
        write8(0);
        break;
      default:
        break;
    }
  }

  void MTPD::getObjectPropValue(uint32_t p1, uint32_t p2)
  { char name[128];
    uint32_t dir;
    uint32_t size;
    uint32_t parent;

    storage_->GetObjectInfo(p1,name,&dir,&size,&parent);

    switch(p2)
    {
      case MTP_PROPERTY_STORAGE_ID:         //0xDC01:
        write32(p1);
        break;
      case MTP_PROPERTY_OBJECT_FORMAT:      //0xDC02:
        write16(dir?0x3001:0x3000);
        break;
      case MTP_PROPERTY_PROTECTION_STATUS:  //0xDC03:
        write16(0);
        break;
      case MTP_PROPERTY_OBJECT_SIZE:        //0xDC04:
        write32(size);
        write32(0);
        break;
      case MTP_PROPERTY_OBJECT_FILE_NAME:   //0xDC07:
        writestring(name);
        break;
      case MTP_PROPERTY_DATE_CREATED:       //0xDC08:
        writestring("");
        break;
      case MTP_PROPERTY_DATE_MODIFIED:      //0xDC09:
        writestring("");
        break;
      case MTP_PROPERTY_PARENT_OBJECT:      //0xDC0B:
        write32(parent);
        break;
      case MTP_PROPERTY_PERSISTENT_UID:     //0xDC41:
        write32(p1);
        write32(parent);
        write32(1);
        write32(0);
        break;
      case MTP_PROPERTY_NAME:               //0xDC44:
        writestring(name);
        break;
      default:
        break;
    }
  }

  uint32_t MTPD::deleteObject(uint32_t p1)
  {
      if (!storage_->DeleteObject(p1))
      {
        return 0x2012; // partial deletion
      }
      return 0x2001;
  }

  uint32_t MTPD::moveObject(uint32_t p1, uint32_t p3)
  { // p1 object
    // p3 new directory
    storage_->move(p1,p3);
    return 0x2001;
  }


#if defined(__IMXRT1062__)

  void MTPD::PrintPacket(const uint8_t *x, int len) 
  {
  #if 0
      for (int i = 0; i < len; i++) {
        printf("%c","0123456789ABCDEF"[x[i] >> 4]);
        printf("%c","0123456789ABCDEF"[x[i] & 0xf]);
        if ((i & 1) == 1) printf(" ");
      }
      printf("\n");
  #endif
  }

  #define CONTAINER ((struct MTPContainer*)(data_buffer))

  #define TRANSMIT(FUN) do {                            \
    write_length_ = 0;                                  \
    write_get_length_ = true;                           \
    FUN;                                                \
    \
    MTPHeader header;                                   \
    header.len = write_length_ + sizeof(header);        \
    header.type = 2;                                    \
    header.op = CONTAINER->op;                          \
    header.transaction_id = CONTAINER->transaction_id;  \
    write_length_ = 0;                                  \
    write_get_length_ = false;                          \
    write((char *)&header, sizeof(header));             \
    FUN;                                                \
    \
    uint32_t rest;                                      \
    rest = (header.len % MTP_TX_SIZE);                  \
    if(rest>0)                                          \
    {                                                   \
      for(int ii=rest;ii<MTP_RX_SIZE; ii++) data_buffer[ii]=0;  \
      PrintPacket(data_buffer,rest);                            \
      usb_mtp_send(data_buffer,rest,30);                        \
    }                                                   \
  } while(0)

  void MTPD::write(const char *data, int len) 
  { if (write_get_length_) 
    {
      write_length_ += len;
    } 
    else 
    { 
      static uint8_t *dst=0;
      if(!write_length_) dst=data_buffer;   write_length_ += len;
      const char * src=data;
      //
      int pos = 0; // into data
      while(pos<len)
      { int avail = data_buffer+MTP_TX_SIZE - dst;
        int to_copy = min(len - pos, avail);
        memcpy(dst,src,to_copy);
        pos += to_copy;
        src += to_copy;
        dst += to_copy;
        if(dst == data_buffer+MTP_TX_SIZE)
        { PrintPacket(data_buffer,MTP_TX_SIZE);
          usb_mtp_send(data_buffer,MTP_TX_SIZE,30);
          dst=data_buffer;
        }
      }
    }
  }
/*
  void MTPD::write8 (uint8_t  x) { write((char*)&x, sizeof(x)); }
  void MTPD::write16(uint16_t x) { write((char*)&x, sizeof(x)); }
  void MTPD::write32(uint32_t x) { write((char*)&x, sizeof(x)); }
  void MTPD::write64(uint64_t x) { write((char*)&x, sizeof(x)); }

  void MTPD::writestring(const char* str) {
    if (*str) 
    { write8(strlen(str) + 1);
      while (*str) {  write16(*str);  ++str;  } write16(0);
    } else 
    { write8(0);
    }
  }

  void MTPD::WriteDescriptor() {
    write16(100);  // MTP version
    write32(6);    // MTP extension
//    write32(0xFFFFFFFFUL);    // MTP extension
    write16(100);  // MTP version
    writestring("microsoft.com: 1.0;");
    write16(0);    // functional mode

    // Supported operations (array of uint16)
    write32(supported_op_num);
    for(int ii=0; ii<supported_op_num;ii++) write16(supported_op[ii]);

    write32(0);       // Events (array of uint16)
    //write16(0x4008);    // Device Info changed // cannot get this working!

    write32(1);       // Device properties (array of uint16)
    write16(0xd402);  // Device friendly name

    write32(0);       // Capture formats (array of uint16)

    write32(2);       // Playback formats (array of uint16)
    write16(0x3000);  // Undefined format
    write16(0x3001);  // Folders (associations)

    writestring("PJRC");     // Manufacturer
    writestring("Teensy");   // Model
    writestring("1.0");      // version
    writestring("???");      // serial
  }

  void MTPD::GetDevicePropValue(uint32_t prop) {
    switch (prop) {
      case 0xd402: // friendly name
        // This is the name we'll actually see in the windows explorer.
        // Should probably be configurable.
        writestring("Teensy");
        break;
    }
  }

  void MTPD::GetDevicePropDesc(uint32_t prop) {
    switch (prop) {
      case 0xd402: // friendly name
        write16(prop);
        write16(0xFFFF); // string type
        write8(0);       // read-only
        GetDevicePropValue(prop);
        GetDevicePropValue(prop);
        write8(0);       // no form
    }
  }

  void MTPD::WriteStorageIDs() {
    write32(1); // 1 entry
    write32(1); // 1 storage
  }

  void MTPD::GetStorageInfo(uint32_t storage) {
    uint16_t readOnly = storage_->readonly();
    uint16_t has_directories = storage_->has_directories();
    uint64_t storage_size = storage_->size();
    uint64_t storage_free = storage_->free();
    
    write16(readOnly ? 0x0001 : 0x0004);   // storage type (removable RAM)
    write16(has_directories ? 0x0002: 0x0001);   // filesystem type (generic hierarchical)
    write16(0x0000);   // access capability (read-write)
    write64(storage_size);  // max capacity
    write64(storage_free);  // free space (100M)
    write32(0xFFFFFFFFUL);  // free space (objects)
    writestring("SD Card");  // storage descriptor
    writestring("");  // volume identifier
  }

  uint32_t MTPD::GetNumObjects(uint32_t storage, uint32_t parent) 
  {
    storage_->StartGetObjectHandles(parent);
    int num = 0;
    while (storage_->GetNextObjectHandle()) num++;
    return num;
  }

  void MTPD::GetObjectHandles(uint32_t storage, uint32_t parent) 
  {
    uint32_t num = 0;
    if (!write_get_length_) {
      num = GetNumObjects(storage, parent);
    }
    write32(num);
    int handle;
    storage_->StartGetObjectHandles(parent);
    while ((handle = storage_->GetNextObjectHandle()))
      write32(handle);
  }

  void MTPD::GetObjectInfo(uint32_t handle) 
  {
    char filename[256];
    uint32_t dir, size, parent;
    storage_->GetObjectInfo(handle, filename, &dir, &size, &parent);

    write32(1); // storage
    write16(dir? 0x3001 : 0x3000); // format
    write16(0);  // protection
    write32(size); // size
    write16(0); // thumb format
    write32(0); // thumb size
    write32(0); // thumb width
    write32(0); // thumb height
    write32(0); // pix width
    write32(0); // pix height
    write32(0); // bit depth
    write32(parent); // parent
    //write16(size == 0xFFFFFFFFUL ? 1 : 0); // association type
    write16(dir);
    write32(0); // association description
    write32(0);  // sequence number
    writestring(filename);
    writestring("");  // date created
    writestring("");  // date modified
    writestring("");  // keywords
  }
*/

  void MTPD::GetObject(uint32_t object_id) 
  {
    uint32_t size = storage_->GetSize(object_id);

    if (write_get_length_) {
      write_length_ += size;
    } else 
    { 
      uint32_t pos = 0; // into data
      uint32_t len = sizeof(MTPHeader);

      disk_pos=DISK_BUFFER_SIZE;
      while(pos<size)
      {
        if(disk_pos==DISK_BUFFER_SIZE)
        {
          uint32_t nread=min(size-pos,(uint32_t)DISK_BUFFER_SIZE);
          //printf("a %d\n",nread);
          storage_->read(object_id,pos,(char *)disk_buffer,nread);
          disk_pos=0;
        }

        uint32_t to_copy = min(size-pos,MTP_TX_SIZE-len);
        to_copy = min (to_copy, DISK_BUFFER_SIZE-disk_pos);

        //printf("b %d %d %d \n",len,disk_pos,to_copy);
        memcpy(data_buffer+len,disk_buffer+disk_pos,to_copy);
        disk_pos += to_copy;
        pos += to_copy;
        len += to_copy;

        if(len==MTP_TX_SIZE)
        { PrintPacket(data_buffer,MTP_TX_SIZE);
          usb_mtp_send(data_buffer,MTP_TX_SIZE,30);
          len=0;
        }
      }
      if(len>0)
      {
        PrintPacket(data_buffer,MTP_TX_SIZE);
        usb_mtp_send(data_buffer,MTP_TX_SIZE,30);
        len=0;
      }
    }
  }

  #define printContainer() {   printf("%x %d %d %d %x %x %x\n", \
              CONTAINER->op, CONTAINER->len, CONTAINER->type, CONTAINER->transaction_id, \
              CONTAINER->params[0], CONTAINER->params[1], CONTAINER->params[2]);  }

  void MTPD::fetch_packet(uint8_t *data_buffer)
  {
        mtp_rx_counter=0;
        usb_mtp_recv(data_buffer,30);
        while(!mtp_rx_counter) mtp_yield(); // wait until data have arrived
        //
        usb_mtp_read(data_buffer,30);
  }

  void MTPD::read(char* data, uint32_t size) 
  {
    static int index=0;
    if(!size) 
    {
      index=0;
      return;
    }

    while (size) {
      uint32_t to_copy = MTP_RX_SIZE - index;
      to_copy = min(to_copy, size);
      if (data) {
        memcpy(data, data_buffer + index, to_copy);
        data += to_copy;
      }
      size -= to_copy;
      index += to_copy;
      if (index == MTP_RX_SIZE) {
        fetch_packet(data_buffer);
        index=0;
      }
    }
  }
/*
  uint32_t MTPD::ReadMTPHeader() {
    MTPHeader header;
    read(0,0);
    read((char *)&header, sizeof(MTPHeader));
    // check that the type is data
    return header.len - 12;
  }

  uint8_t MTPD::read8() {
    uint8_t ret;
    read((char*)&ret, sizeof(ret));
    return ret;
  }

  uint16_t MTPD::read16() {
    uint16_t ret;
    read((char*)&ret, sizeof(ret));
    return ret;
  }

  uint32_t MTPD::read32() {
    uint32_t ret;
    read((char*)&ret, sizeof(ret));
    return ret;
  }

  uint32_t MTPD::readstring(char* buffer) {
    int len = read8();
    if (!buffer) {
      read(NULL, len * 2);
    } else {
      for (int i = 0; i < len; i++) {
        *(buffer++) = read16();
      }
    }
    *(buffer++)=0; *(buffer++)=0; // to be sure
    return 2*len+1;
  }
*/
  void MTPD::read_until_short_packet() {
    bool done = false;
    while (!done) {
        fetch_packet(data_buffer);
        done = CONTAINER->len < MTP_RX_SIZE;
    }
  }

  uint32_t MTPD::SendObjectInfo(uint32_t storage, uint32_t parent) {
    fetch_packet(data_buffer);

    int len=ReadMTPHeader();
    char filename[256];

    read32(); len -=4; // storage
    bool dir = (read16() == 0x3001); len -=2; // format
    read16(); len -=2; // protection
    read32(); len -=4; // size
    read16(); len -=2; // thumb format
    read32(); len -=4; // thumb size
    read32(); len -=4; // thumb width
    read32(); len -=4; // thumb height
    read32(); len -=4; // pix width
    read32(); len -=4; // pix height
    read32(); len -=4; // bit depth
    read32(); len -=4; // parent
    read16(); len -=2; // association type
    read32(); len -=4; // association description
    read32(); len -=4; // sequence number

    len -=readstring(filename); len -=strlen(filename); 

    if(len>MTP_RX_SIZE)  read_until_short_packet();  // ignores dates & keywords
    return storage_->Create(parent, dir, filename);
  }

  void MTPD::SendObject() {
    fetch_packet(data_buffer);
    //printContainer(); 

    uint32_t len=ReadMTPHeader();
    uint32_t index = sizeof(MTPHeader);
    //int old=len;
    while(len>0)
    { uint32_t bytes = MTP_RX_SIZE - index;                     // how many data in usb-packet
      bytes = min(bytes,len);                                   // loimit at end
      uint32_t to_copy=min(bytes, DISK_BUFFER_SIZE-disk_pos);   // how many data to copy to disk buffer
      memcpy(disk_buffer+disk_pos, data_buffer + index,to_copy);
      disk_pos += to_copy;
      bytes -= to_copy;
      len -= to_copy;
      //printf("a %d %d %d %d %d\n", len,disk_pos,bytes,index,to_copy);
      //
      if(disk_pos==DISK_BUFFER_SIZE)
      {
        storage_->write((const char *)disk_buffer, DISK_BUFFER_SIZE);
        disk_pos =0;

        if(bytes) // we have still data in transfer buffer, copy to initial disk_buffer
        {
          memcpy(disk_buffer,data_buffer+index+to_copy,bytes);
          disk_pos += bytes;
          len -= bytes;
        }
        //printf("b %d %d %d %d %d\n", len,disk_pos,bytes,index,to_copy);
      }
      if(len>0)  // we have still data to be transfered
      {
        fetch_packet(data_buffer);
        index=0;
      }
    }
    //printf("len %d\n",disk_pos);
    if(disk_pos)
    {
      storage_->write((const char *)disk_buffer, disk_pos);
    }
    storage_->close();
  }
/*
#define MTP_PROPERTY_STORAGE_ID                             0xDC01
#define MTP_PROPERTY_OBJECT_FORMAT                          0xDC02
#define MTP_PROPERTY_PROTECTION_STATUS                      0xDC03
#define MTP_PROPERTY_OBJECT_SIZE                            0xDC04
#define MTP_PROPERTY_OBJECT_FILE_NAME                       0xDC07
#define MTP_PROPERTY_DATE_CREATED                           0xDC08
#define MTP_PROPERTY_DATE_MODIFIED                          0xDC09
#define MTP_PROPERTY_PARENT_OBJECT                          0xDC0B
#define MTP_PROPERTY_PERSISTENT_UID                         0xDC41
#define MTP_PROPERTY_NAME                                   0xDC44

const uint16_t propertyList[] =
{
  MTP_PROPERTY_STORAGE_ID                             ,//0xDC01
  MTP_PROPERTY_OBJECT_FORMAT                          ,//0xDC02
  MTP_PROPERTY_PROTECTION_STATUS                      ,//0xDC03
  MTP_PROPERTY_OBJECT_SIZE                            ,//0xDC04
  MTP_PROPERTY_OBJECT_FILE_NAME                       ,//0xDC07
  MTP_PROPERTY_DATE_CREATED                           ,//0xDC08
  MTP_PROPERTY_DATE_MODIFIED                          ,//0xDC09
  MTP_PROPERTY_PARENT_OBJECT                          ,//0xDC0B
  MTP_PROPERTY_PERSISTENT_UID                         ,//0xDC41
  MTP_PROPERTY_NAME                                    //0xDC44
};

uint32_t propertyListNum = sizeof(propertyList)/sizeof(propertyList[0]);

  void MTPD::getObjectPropsSupported(uint32_t p1)
  {
    write32(propertyListNum);
    for(uint32_t ii=0; ii<propertyListNum;ii++) write16(propertyList[ii]);
  }

  void MTPD::getObjectPropDesc(uint32_t p1, uint32_t p2)
  {
    switch(p1)
    {
      case MTP_PROPERTY_STORAGE_ID:         //0xDC01:
        write16(0xDC01);
        write16(0x006);
        write8(0); //get
        write32(0);
        write32(0);
        write8(0);
        break;
      case MTP_PROPERTY_OBJECT_FORMAT:        //0xDC02:
        write16(0xDC02);
        write16(0x004);
        write8(0); //get
        write16(0);
        write32(0);
        write8(0);
        break;
      case MTP_PROPERTY_PROTECTION_STATUS:    //0xDC03:
        write16(0xDC03);
        write16(0x004);
        write8(0); //get
        write16(0);
        write32(0);
        write8(0);
        break;
      case MTP_PROPERTY_OBJECT_SIZE:        //0xDC04:
        write16(0xDC04);
        write16(0x008);
        write8(0); //get
        write64(0);
        write32(0);
        write8(0);
        break;
      case MTP_PROPERTY_OBJECT_FILE_NAME:   //0xDC07:
        write16(0xDC07);
        write16(0xFFFF);
        write8(1); //get/set
        write8(0);
        write32(0);
        write8(0);
        break;
      case MTP_PROPERTY_DATE_CREATED:       //0xDC08:
        write16(0xDC08);
        write16(0xFFFF);
        write8(0); //get
        write8(0);
        write32(0);
        write8(0);
        break;
      case MTP_PROPERTY_DATE_MODIFIED:      //0xDC09:
        write16(0xDC09);
        write16(0xFFFF);
        write8(0); //get
        write8(0);
        write32(0);
        write8(0);
        break;
      case MTP_PROPERTY_PARENT_OBJECT:    //0xDC0B:
        write16(0xDC0B);
        write16(6);
        write8(0); //get
        write32(0);
        write32(0);
        write8(0);
        break;
      case MTP_PROPERTY_PERSISTENT_UID:   //0xDC41:
        write16(0xDC41);
        write16(0x0A);
        write8(0); //get
        write64(0);
        write64(0);
        write32(0);
        write8(0);
        break;
      case MTP_PROPERTY_NAME:             //0xDC44:
        write16(0xDC44);
        write16(0xFFFF);
        write8(0); //get
        write8(0);
        write32(0);
        write8(0);
        break;
      default:
        break;
    }
  }

  void MTPD::getObjectPropValue(uint32_t p1, uint32_t p2)
  { char name[128];
    uint32_t dir;
    uint32_t size;
    uint32_t parent;

    storage_->GetObjectInfo(p1,name,&dir,&size,&parent);

    switch(p2)
    {
      case MTP_PROPERTY_STORAGE_ID:         //0xDC01:
        write32(p1);
        break;
      case MTP_PROPERTY_OBJECT_FORMAT:      //0xDC02:
        write16(dir?0x3001:0x3000);
        break;
      case MTP_PROPERTY_PROTECTION_STATUS:  //0xDC03:
        write16(0);
        break;
      case MTP_PROPERTY_OBJECT_SIZE:        //0xDC04:
        write32(size);
        write32(0);
        break;
      case MTP_PROPERTY_OBJECT_FILE_NAME:   //0xDC07:
        writestring(name);
        break;
      case MTP_PROPERTY_DATE_CREATED:       //0xDC08:
        writestring("");
        break;
      case MTP_PROPERTY_DATE_MODIFIED:      //0xDC09:
        writestring("");
        break;
      case MTP_PROPERTY_PARENT_OBJECT:      //0xDC0B:
        write32(parent);
        break;
      case MTP_PROPERTY_PERSISTENT_UID:     //0xDC41:
        write32(p1);
        write32(parent);
        write32(1);
        write32(0);
        break;
      case MTP_PROPERTY_NAME:               //0xDC44:
        writestring(name);
        break;
      default:
        break;
    }
  }

  uint32_t MTPD::setObjectPropValue(uint32_t p1, uint32_t p2)
  {
    fetch_packet(data_buffer);
    printContainer(); 
    
    if(p2==0xDC07)
    {
      char filename[128];
      ReadMTPHeader();
      readstring(filename);

      storage_->rename(p1,filename);

      return 0x2001;
    }
    else
      return 0x2005;
  }

  uint32_t MTPD::deleteObject(uint32_t p1)
  {
      if (!storage_->DeleteObject(p1))
      {
        return 0x2012; // partial deletion
      }
      return 0x2001;
  }

  uint32_t MTPD::moveObject(uint32_t p1, uint32_t p3)
  { // p1 object
    // p3 new directory
    storage_->move(p1,p3);
    return 0x2001;
  }
*/

  uint32_t MTPD::setObjectPropValue(uint32_t p1, uint32_t p2)
  {
    fetch_packet(data_buffer);
    printContainer(); 
    
    if(p2==0xDC07)
    {
      char filename[128];
      ReadMTPHeader();
      readstring(filename);

      storage_->rename(p1,filename);

      return 0x2001;
    }
    else
      return 0x2005;
  }



  void MTPD::loop(void)
  {
    if(mtp_rx_counter>0)
    { 
      usb_mtp_read(data_buffer,30);
      printf("op=%x typ=%d tid=%d len=%d\n", CONTAINER->op, CONTAINER->type, CONTAINER->transaction_id, CONTAINER->len);
      PrintPacket(data_buffer,CONTAINER->len);

      int op = CONTAINER->op;
      int p1 = CONTAINER->params[0];
      int p2 = CONTAINER->params[1];
      int p3 = CONTAINER->params[2];
      int id = CONTAINER->transaction_id;
      int len= CONTAINER->len;
      int typ= CONTAINER->type;

      int return_code =0x2001; //OK use as default value

      if(typ==2) return_code=0x2005; // we should only get cmds

      switch (op)
      {
        case 0x1001:
          p1=0;
          TRANSMIT(WriteDescriptor());
          break;

        case 0x1002:  //open session
          //
          break;

        case 0x1003:  // CloseSession
          //
          break;

        case 0x1004:  // GetStorageIDs
            TRANSMIT(WriteStorageIDs());
          break;

        case 0x1005:  // GetStorageInfo
          TRANSMIT(GetStorageInfo(p1));
          break;

        case 0x1006:  // GetNumObjects
          if (CONTAINER->params[1]) 
          {
              return_code = 0x2014; // spec by format unsupported
          } else 
          {
              p1 = GetNumObjects(p1, p3);
          }
          break;

        case 0x1007:  // GetObjectHandles
          //printContainer(); 

          if (p2) 
          { return_code = 0x2014; // spec by format unsupported
          } else 
          { 
            TRANSMIT(GetObjectHandles(p1, p3));
          }
          break;

        case 0x1008:  // GetObjectInfo
          TRANSMIT(GetObjectInfo(p1));
           break;

        case 0x1009:  // GetObject
           TRANSMIT(GetObject(p1));
           break;

        case 0x100B:  // DeleteObject
           if (CONTAINER->len>16 && p2) 
           {
              return_code = 0x2014; // spec by format unsupported
           } else 
           {  return_code = deleteObject(p1);
           }
            CONTAINER->len  = len = 12;
           break;

        case 0x100C:  // SendObjectInfo
            if (!p1) p1 = 1;
            CONTAINER->params[2] = SendObjectInfo(p1, // storage
                                                  p2); // parent

            CONTAINER->params[1]=p2;
            CONTAINER->len  = len = 12 + 3 * 4;
            break;

        case 0x100D:  // SendObject
            SendObject();
            CONTAINER->len  = len = 12;
            break;

        case 0x1014:  // GetDevicePropDesc
            TRANSMIT(GetDevicePropDesc(p1));
            break;

        case 0x1015:  // GetDevicePropvalue
            TRANSMIT(GetDevicePropValue(p1));
            break;

        case 0x1010:  // Reset
            return_code = 0x2005;
            break;

        case 0x1019:  // MoveObject
            return_code = moveObject(p1,p3);
            CONTAINER->len  = len = 12;
            break;

        case 0x101A:  // CopyObject
            return_code = 0x2005;
            break;

        case 0x9801:  // getObjectPropsSupported
        printContainer(); 
            TRANSMIT(getObjectPropsSupported(p1));
            break;

        case 0x9802:  // getObjectPropDesc
        printContainer(); 
        TRANSMIT(getObjectPropDesc(p1,p2));
            break;

        case 0x9803:  // getObjectPropertyValue
        printContainer(); 
          TRANSMIT(getObjectPropValue(p1,p2));
            break;

        case 0x9804:  // setObjectPropertyValue
        printContainer(); 
            return_code = setObjectPropValue(p1,p2);
            break;

        default:
            return_code = 0x2005;  // operation not supported
            break;
      }
      if(return_code)
      {
          CONTAINER->type=3;
          CONTAINER->len=len;
          CONTAINER->op=return_code;
          CONTAINER->transaction_id=id;
          CONTAINER->params[0]=p1;
          PrintPacket(data_buffer,len);
          usb_mtp_send(data_buffer,len,30);
      }
      mtp_rx_counter=0;
      usb_mtp_recv(data_buffer,30);
    }

    if(mtp_tx_counter>0) { mtp_tx_counter--; }
    if(mtp_rx_event_counter>0)  { mtp_rx_event_counter--; }
    if(mtp_tx_event_counter>0)  { mtp_tx_event_counter--; }
  }


#else /******************************************************** Teensy 3 ********************************/

  #include "usb_desc.h"
  void MTPD::PrintPacket(const usb_packet_t *x) {
#if 0
    for (int i = 0; i < x->len; i++) {
      Serial1.print("0123456789ABCDEF"[x->buf[i] >> 4]);
      Serial1.print("0123456789ABCDEF"[x->buf[i] & 0xf]);
      if ((i & 1) == 1) Serial1.print(" ");
    }
    Serial1.println("");
#endif
#if 1
    MTPContainer *tmp = (struct MTPContainer*)(x->buf);
    if(tmp->type==1)
    {
      Serial1.print(" len = ");
      Serial1.print(tmp->len);
      Serial1.print(" type = ");
      Serial1.print(tmp->type);
      Serial1.print(" op = ");
      Serial1.print(tmp->op, HEX);
      Serial1.print(" transaction_id = ");
      Serial1.print(tmp->transaction_id);
      for (int i = 0; i * 4 < x->len - 12; i ++) {
        Serial1.print(" p");
        Serial1.print(i);
        Serial1.print(" = ");
        Serial1.print(tmp->params[i], HEX);
      }
      Serial1.println("");
    }
#endif
  }

  void MTPD::get_buffer() {
    while (!data_buffer_) {
      data_buffer_ = usb_malloc();
      if (!data_buffer_) mtp_yield();
    }
  }

  void MTPD::receive_buffer() {
    while (!data_buffer_) {
      data_buffer_ = usb_rx(MTP_RX_ENDPOINT);
      if (!data_buffer_) mtp_yield();
    }
  }

  void MTPD::write(const char *data, int len) {
    if (write_get_length_) {
      write_length_ += len;
    } else {
      int pos = 0;
      while (pos < len) {
        get_buffer();
        int avail = sizeof(data_buffer_->buf) - data_buffer_->len;
        int to_copy = min(len - pos, avail);
        memcpy(data_buffer_->buf + data_buffer_->len,
               data + pos,
               to_copy);
        data_buffer_->len += to_copy;
        pos += to_copy;
        if (data_buffer_->len == sizeof(data_buffer_->buf)) {
          PrintPacket(data_buffer_);
          usb_tx(MTP_TX_ENDPOINT, data_buffer_);
          data_buffer_ = NULL;
          // Serial1.println("SENT...");
        }
      }
    }
  }
/*
  void MTPD::write8 (uint8_t  x) { write((char*)&x, sizeof(x)); }
  void MTPD::write16(uint16_t x) { write((char*)&x, sizeof(x)); }
  void MTPD::write32(uint32_t x) { write((char*)&x, sizeof(x)); }
  void MTPD::write64(uint64_t x) { write((char*)&x, sizeof(x)); }

  void MTPD::writestring(const char* str) {
    if (*str) 
    { write8(strlen(str) + 1);
      while (*str) {  write16(*str);  ++str;  } write16(0);
    } else 
    { write8(0);
    }
  }

  void MTPD::WriteDescriptor() {
    write16(100);  // MTP version
    write32(6);    // MTP extension
//    write32(0xFFFFFFFFUL);    // MTP extension
    write16(100);  // MTP version
    writestring("microsoft.com: 1.0;");
    write16(0);    // functional mode

    // Supported operations (array of uint16)
    write32(14);
    write16(0x1001);  // GetDeviceInfo
    write16(0x1002);  // OpenSession
    write16(0x1003);  // CloseSession
    write16(0x1004);  // GetStorageIDs

    write16(0x1005);  // GetStorageInfo
    write16(0x1006);  // GetNumObjects
    write16(0x1007);  // GetObjectHandles
    write16(0x1008);  // GetObjectInfo

    write16(0x1009);  // GetObject
    write16(0x100B);  // DeleteObject
    write16(0x100C);  // SendObjectInfo
    write16(0x100D);  // SendObject

    write16(0x1014);  // GetDevicePropDesc
    write16(0x1015);  // GetDevicePropValue

//    write16(0x1010);  // Reset
//    write16(0x1019);  // MoveObject
//    write16(0x101A);  // CopyObject

    write32(0);       // Events (array of uint16)

    write32(1);       // Device properties (array of uint16)
    write16(0xd402);  // Device friendly name

    write32(0);       // Capture formats (array of uint16)

    write32(2);       // Playback formats (array of uint16)
    write16(0x3000);  // Undefined format
    write16(0x3001);  // Folders (associations)

    writestring("PJRC");     // Manufacturer
    writestring("Teensy");   // Model
    writestring("1.0");      // version
    writestring("???");      // serial
  }

  void MTPD::WriteStorageIDs() {
    write32(1); // 1 entry
    write32(1); // 1 storage
  }

  void MTPD::GetStorageInfo(uint32_t storage) {
    write16(storage_->readonly() ? 0x0001 : 0x0004);   // storage type (removable RAM)
    write16(storage_->has_directories() ? 0x0002: 0x0001);   // filesystem type (generic hierarchical)
    write16(0x0000);   // access capability (read-write)
    write64(storage_->size());  // max capacity
    write64(storage_->free());  // free space (100M)
    write32(0xFFFFFFFFUL);  // free space (objects)
    writestring("SD Card");  // storage descriptor
    writestring("");  // volume identifier
  }

  uint32_t MTPD::GetNumObjects(uint32_t storage, uint32_t parent) 
  {
    storage_->StartGetObjectHandles(parent);
    int num = 0;
    while (storage_->GetNextObjectHandle()) num++;
    return num;
  }

  void MTPD::GetObjectHandles(uint32_t storage, uint32_t parent) 
  {
    uint32_t num = 0;
    if (!write_get_length_) {
      num = GetNumObjects(storage, parent);
    }
    write32(num);
    int handle;
    storage_->StartGetObjectHandles(parent);
    while ((handle = storage_->GetNextObjectHandle()))
      write32(handle);
  }

  void MTPD::GetObjectInfo(uint32_t handle) 
  {
    char filename[256];
    uint32_t size, dir, parent;
    storage_->GetObjectInfo(handle, filename, &dir, &size, &parent);

    write32(1); // storage
    write16(dir ? 0x3001 : 0x3000); // format
    write16(0);  // protection
    write32(size); // size
    write16(0); // thumb format
    write32(0); // thumb size
    write32(0); // thumb width
    write32(0); // thumb height
    write32(0); // pix width
    write32(0); // pix height
    write32(0); // bit depth
    write32(parent); // parent
    write16(dir); // association type
    write32(0); // association description
    write32(0);  // sequence number
    writestring(filename);
    writestring("");  // date created
    writestring("");  // date modified
    writestring("");  // keywords
  }
*/
  void MTPD::GetObject(uint32_t object_id) 
  {
    uint32_t size = storage_->GetSize(object_id);
    if (write_get_length_) {
      write_length_ += size;
    } else {
      uint32_t pos = 0;
      while (pos < size) {
        get_buffer();
        uint32_t avail = sizeof(data_buffer_->buf) - data_buffer_->len;
        uint32_t to_copy = min(size - pos, avail);
        // Read directly from storage into usb buffer.
        storage_->read(object_id, pos,
                    (char*)(data_buffer_->buf + data_buffer_->len),
                    to_copy);
        pos += to_copy;
        data_buffer_->len += to_copy;
        if (data_buffer_->len == sizeof(data_buffer_->buf)) {
          usb_tx(MTP_TX_ENDPOINT, data_buffer_);
          data_buffer_ = NULL;
        }
      }
    }
  }


#define CONTAINER contains(receive_buffer)

#define TRANSMIT(FUN) do {                              \
    write_length_ = 0;                                  \
    write_get_length_ = true;                           \
    FUN;                                                \
    write_get_length_ = false;                          \
    MTPHeader header;                                   \
    header.len = write_length_ + 12;                    \
    header.type = 2;                                    \
    header.op = CONTAINER->op;                          \
    header.transaction_id = CONTAINER->transaction_id;  \
    write((char *)&header, sizeof(header));             \
    FUN;                                                \
    get_buffer();                                       \
    PrintPacket(data_buffer_);                        \
    usb_tx(MTP_TX_ENDPOINT, data_buffer_);              \
    data_buffer_ = NULL;                                \
  } while(0)

  void MTPD::read(char* data, uint32_t size) 
  {
    while (size) {
      receive_buffer();
      uint32_t to_copy = data_buffer_->len - data_buffer_->index;
      to_copy = min(to_copy, size);
      if (data) {
        memcpy(data, data_buffer_->buf + data_buffer_->index, to_copy);
        data += to_copy;
      }
      size -= to_copy;
      data_buffer_->index += to_copy;
      if (data_buffer_->index == data_buffer_->len) {
        usb_free(data_buffer_);
        data_buffer_ = NULL;
      }
    }
  }
/*
  uint32_t MTPD::ReadMTPHeader() 
  {
    MTPHeader header;
    read((char *)&header, sizeof(MTPHeader));
    // check that the type is data
    return header.len - 12;
  }

  uint8_t MTPD::read8() { uint8_t ret; read((char*)&ret, sizeof(ret));  return ret;  }
  uint16_t MTPD::read16() { uint16_t ret; read((char*)&ret, sizeof(ret)); return ret; }
  uint32_t MTPD::read32() { uint32_t ret; read((char*)&ret, sizeof(ret)); return ret; }

  uint32_t MTPD::readstring(char* buffer) {
    int len = read8();
    if (!buffer) {
      read(NULL, len * 2);
    } else {
      for (int i = 0; i < len; i++) {
        *(buffer++) = read16();
      }
    }
    return 2*len+1;
  }
*/
  void MTPD::read_until_short_packet() {
    bool done = false;
    while (!done) {
      receive_buffer();
      done = data_buffer_->len != sizeof(data_buffer_->buf);
      usb_free(data_buffer_);
      data_buffer_ = NULL;
    }
  }

  uint32_t MTPD::SendObjectInfo(uint32_t storage, uint32_t parent) {
    ReadMTPHeader();
    char filename[256];

    read32(); // storage
    bool dir = read16() == 0x3001; // format
    read16();  // protection
    read32(); // size
    read16(); // thumb format
    read32(); // thumb size
    read32(); // thumb width
    read32(); // thumb height
    read32(); // pix width
    read32(); // pix height
    read32(); // bit depth
    read32(); // parent
    read16(); // association type
    read32(); // association description
    read32(); // sequence number

    readstring(filename);
    read_until_short_packet();  // ignores dates & keywords
    return storage_->Create(parent, dir, filename);
  }

  void MTPD::SendObject() {
    uint32_t len = ReadMTPHeader();
    while (len) 
    {
      receive_buffer();
      uint32_t to_copy = data_buffer_->len - data_buffer_->index;
      to_copy = min(to_copy, len);
      storage_->write((char*)(data_buffer_->buf + data_buffer_->index), to_copy);
      data_buffer_->index += to_copy;
      len -= to_copy;
      if (data_buffer_->index == data_buffer_->len) 
      {
        usb_free(data_buffer_);
        data_buffer_ = NULL;
      }
    }
    storage_->close();
  }

/*
  void MTPD::GetDevicePropValue(uint32_t prop) {
    switch (prop) {
      case 0xd402: // friendly name
        // This is the name we'll actually see in the windows explorer.
        // Should probably be configurable.
        writestring("Teensy");
        break;
    }
  }

  void MTPD::GetDevicePropDesc(uint32_t prop) {
    switch (prop) {
      case 0xd402: // friendly name
        write16(prop);
        write16(0xFFFF); // string type
        write8(0);       // read-only
        GetDevicePropValue(prop);
        GetDevicePropValue(prop);
        write8(0);       // no form
    }
  }
*/
  uint32_t MTPD::setObjectPropValue(uint32_t p1, uint32_t p2)
  {
    receive_buffer();

    if(p2==0xDC07)
    {
      char filename[128];
      ReadMTPHeader();
      readstring(filename);

      storage_->rename(p1,filename);

      return 0x2001;
    }
    else
      return 0x2005;
  }


  void MTPD::loop() 
  {
    usb_packet_t *receive_buffer;
    if ((receive_buffer = usb_rx(MTP_RX_ENDPOINT))) {
      PrintPacket(receive_buffer);

      int op = CONTAINER->op;
      int p1 = CONTAINER->params[0];
      int p2 = CONTAINER->params[1];
      int p3 = CONTAINER->params[2];
      int id = CONTAINER->transaction_id;
      int len= CONTAINER->len;
      int typ= CONTAINER->type;

      int return_code =0x2001; //OK use as default value

      if(typ==2) return_code=0x2005; // we should only get cmds

      switch (op)
      {
        case 0x1001:
          p1=0;
          TRANSMIT(WriteDescriptor());
          break;

        case 0x1002:  //open session
          //
          break;

        case 0x1003:  // CloseSession
          //
          break;

        case 0x1004:  // GetStorageIDs
            TRANSMIT(WriteStorageIDs());
          break;

        case 0x1005:  // GetStorageInfo
          TRANSMIT(GetStorageInfo(p1));
          break;

        case 0x1006:  // GetNumObjects
          if (CONTAINER->params[1]) 
          {
              return_code = 0x2014; // spec by format unsupported
          } else 
          {
              p1 = GetNumObjects(p1, p3);
          }
          break;

        case 0x1007:  // GetObjectHandles
          //printContainer(); 

          if (p2) 
          { return_code = 0x2014; // spec by format unsupported
          } else 
          { 
            TRANSMIT(GetObjectHandles(p1, p3));
          }
          break;

        case 0x1008:  // GetObjectInfo
          TRANSMIT(GetObjectInfo(p1));
           break;

        case 0x1009:  // GetObject
           TRANSMIT(GetObject(p1));
           break;

        case 0x100B:  // DeleteObject
           if (CONTAINER->len>16 && p2) 
           {
              return_code = 0x2014; // spec by format unsupported
           } else 
           {  return_code = deleteObject(p1);
           }
            CONTAINER->len  = len = 12;
           break;

        case 0x100C:  // SendObjectInfo
            if (!p1) p1 = 1;
            CONTAINER->params[2] = SendObjectInfo(p1, // storage
                                                  p2); // parent

            CONTAINER->params[1]=p2;
            CONTAINER->len  = len = 12 + 3 * 4;
            break;

        case 0x100D:  // SendObject
            SendObject();
            CONTAINER->len  = len = 12;
            break;

        case 0x1014:  // GetDevicePropDesc
            TRANSMIT(GetDevicePropDesc(p1));
            break;

        case 0x1015:  // GetDevicePropvalue
            TRANSMIT(GetDevicePropValue(p1));
            break;

        case 0x1010:  // Reset
            return_code = 0x2005;
            break;

        case 0x1019:  // MoveObject
            return_code = moveObject(p1,p3);
            CONTAINER->len  = len = 12;
            break;

        case 0x101A:  // CopyObject
            return_code = 0x2005;
            break;

        case 0x9801:  // getObjectPropsSupported

            TRANSMIT(getObjectPropsSupported(p1));
            break;

        case 0x9802:  // getObjectPropDesc

        TRANSMIT(getObjectPropDesc(p1,p2));
            break;

        case 0x9803:  // getObjectPropertyValue

          TRANSMIT(getObjectPropValue(p1,p2));
            break;

        case 0x9804:  // setObjectPropertyValue

            return_code = setObjectPropValue(p1,p2);
            break;

        default:
            return_code = 0x2005;  // operation not supported
            break;
      }

/*
      uint32_t return_code = 0;
      uint32_t p1 = 0;
      if (receive_buffer->len >= 12) {
        return_code = 0x2001;  // Ok
        receive_buffer->len = 16;
        if (CONTAINER->type == 1) { // command
          switch (CONTAINER->op) {
            case 0x1001: // GetDescription
              TRANSMIT(WriteDescriptor());
              break;
            case 0x1002:  // OpenSession
              break;
            case 0x1003:  // CloseSession
              break;
            case 0x1004:  // GetStorageIDs
              TRANSMIT(WriteStorageIDs());
              break;
            case 0x1005:  // GetStorageInfo
              TRANSMIT(GetStorageInfo(CONTAINER->params[0]));
              break;
            case 0x1006:  // GetNumObjects
              if (CONTAINER->params[1]) {
                return_code = 0x2014; // spec by format unsupported
              } else {
                p1 = GetNumObjects(CONTAINER->params[0],
                CONTAINER->params[2]);
              }
              break;
            case 0x1007:  // GetObjectHandles
              if (CONTAINER->params[1]) {
                return_code = 0x2014; // spec by format unsupported
              } else {
                TRANSMIT(GetObjectHandles(CONTAINER->params[0],
                CONTAINER->params[2]));
              }
              break;
            case 0x1008:  // GetObjectInfo
              TRANSMIT(GetObjectInfo(CONTAINER->params[0]));
              break;
            case 0x1009:  // GetObject
              TRANSMIT(GetObject(CONTAINER->params[0]));
              break;
            case 0x100B:  // DeleteObject
              if (CONTAINER->params[1]) {
                return_code = 0x2014; // spec by format unsupported
              } else {
                if (!storage_->DeleteObject(CONTAINER->params[0])) {
                  return_code = 0x2012; // partial deletion
                }
              }
              break;
            case 0x100C:  // SendObjectInfo
              CONTAINER->params[2] =
                  SendObjectInfo(CONTAINER->params[0], // storage
                                 CONTAINER->params[1]); // parent
                  p1 = CONTAINER->params[0];
              if (!p1) p1 = 1;
              CONTAINER->len = receive_buffer->len = 12 + 3 * 4;
              break;
            case 0x100D:  // SendObject
              SendObject();
              break;
            case 0x1014:  // GetDevicePropDesc
              TRANSMIT(GetDevicePropDesc(CONTAINER->params[0]));
              break;
            case 0x1015:  // GetDevicePropvalue
              TRANSMIT(GetDevicePropValue(CONTAINER->params[0]));
              break;
            default:
              return_code = 0x2005;  // operation not supported
              break;
          }
        } else {
          return_code = 0x2000;  // undefined
        }
      }
      */
      if (return_code) {
          CONTAINER->type=3;
          CONTAINER->len=len;
          CONTAINER->op=return_code;
          CONTAINER->transaction_id=id;
          CONTAINER->params[0]=p1;

//        CONTAINER->type = 3;
//        CONTAINER->op = return_code;
//        CONTAINER->params[0] = p1;

        
        PrintPacket(receive_buffer);
        usb_tx(MTP_TX_ENDPOINT, receive_buffer);
        receive_buffer = 0;
      } else {
          usb_free(receive_buffer);
      }
    }
    // Maybe put event handling inside mtp_yield()?
    if ((receive_buffer = usb_rx(MTP_EVENT_ENDPOINT))) {
      usb_free(receive_buffer);
    }
  }

#endif

/*
op=00001008 len=16
op=0000100C len=20
0000100C 00000001 FFFFFFFF FFFFFFFF

op=0000100C len=184
0000100C 00000000 00003000 00000007
New Text Document.txt
op=0000100D len=12
op=0000100D len=19
op=00001005 len=16
op=0000100B len=20
0000100B 00000010 00000000
op=00001005 len=16


op=00001008 len=16
op=00001008 len=16
op=0000100C len=20
0000100C 00000001 00000007 00000007
00000007 0
op=0000100C len=184
0000100C 00000000 00003000 00000007
00003000 0 New Text Document.txt
op=0000100D len=12
op=0000100D len=19
7
op=00001005 len=16


op=0000100C len=20
0000100C 00000001 00000007 00000007
00000007 0 New Text Document.txt
op=0000100C len=184
0000100C 00000000 00003000 00000009
00003000 0 New Text Document.txt
op=00001005 len=16

op=0000100C len=20
0000100C 00000001 FFFFFFFF 00000009
FFFFFFFF 0 New Text Document.txt
op=0000100C len=184
0000100C 00000000 00003000 00000009
00003000 0 New Text Document.txt
op=00001005 len=16

*/