/*=========================================================================

  Program:   DICOMParser
  Module:    $RCSfile: DICOMFile.cxx,v $
  Language:  C++
  Date:      $Date: 2003/10/23 19:13:34 $
  Version:   $Revision: 1.1.1.1 $

  Copyright (c) 2003 Matt Turek
  All rights reserved.
  See Copyright.txt for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifdef _MSC_VER
#pragma warning ( disable : 4514 )
#pragma warning ( disable : 4710 )
#pragma warning ( push, 3 )
#endif 

#include <stdio.h>
#include <cstring>

#include "DICOMConfig.h"
#include "DICOMFile.h"

DICOMFile::DICOMFile() : InputStream()
{
  /* Are we little or big endian?  From Harbison&Steele.  */
  union
  {
    long l;
    char c[sizeof (long)];
  } u;
  u.l = 1;
  PlatformIsBigEndian = (u.c[sizeof (long) - 1] == 1);
  if (PlatformIsBigEndian)
    {
    PlatformEndian = "BigEndian";
    }
  else
    {
    PlatformEndian = "LittleEndian";
    }
}

DICOMFile::~DICOMFile()
{
  this->Close();
}

DICOMFile::DICOMFile(const DICOMFile& in)
{
  if (strcmp(in.PlatformEndian, "LittleEndian") == 0)
    {
    PlatformEndian = "LittleEndian";
    }
  else
    {
    PlatformEndian = "BigEndian";
    }
  //
  // Some compilers can't handle. Comment out for now.
  //
  // InputStream = in.InputStream;
}

void DICOMFile::operator=(const DICOMFile& in)
{
  if (strcmp(in.PlatformEndian, "LittleEndian") == 0)
    {
    PlatformEndian = "LittleEndian";
    }
  else
    {
    PlatformEndian = "BigEndian";
    }
  //
  // Some compilers can't handle. Comment out for now.
  //
  // InputStream = in.InputStream;
}

bool DICOMFile::Open(const dicom_stl::string& filename)
{
#ifdef _WIN32
  InputStream.open(filename.c_str(), dicom_stream::ios::binary | dicom_stream::ios::in);
#else  
  InputStream.open(filename.c_str(), std::ios::in);
#endif

  //if (InputStream.is_open())
  if (InputStream.rdbuf()->is_open())
    {
    return true;
    }
  else
    {
    return false;
    }
}

void DICOMFile::Close()
{
  InputStream.close();
}

long DICOMFile::Tell() 
{
  long loc = InputStream.tellg();
  // dicom_stream::cout << "Tell: " << loc << dicom_stream::endl;
  return loc;
}

void DICOMFile::SkipToPos(long increment) 
{
  InputStream.seekg(increment, std::ios::beg);
}

long DICOMFile::GetSize() 
{
  long curpos = this->Tell();

  InputStream.seekg(0,std::ios::end);

  long size = this->Tell();
  // dicom_stream::cout << "Tell says size is: " << size << dicom_stream::endl;
  this->SkipToPos(curpos);

  return size;
}

void DICOMFile::Skip(long increment) 
{
  InputStream.seekg(increment, std::ios::cur);
}

void DICOMFile::SkipToStart() 
{
  InputStream.seekg(0, std::ios::beg);
}

void DICOMFile::Read(void* ptr, long nbytes) 
{
  InputStream.read((char*)ptr, nbytes);
  // dicom_stream::cout << (char*) ptr << dicom_stream::endl;
}

doublebyte DICOMFile::ReadDoubleByte() 
{
  doublebyte sh = 0;
  int sz = sizeof(doublebyte);
  this->Read((char*)&(sh),sz); 
  if (PlatformIsBigEndian) 
    {
    sh = swapShort(sh);
    }
  return(sh);
}

doublebyte DICOMFile::ReadDoubleByteAsLittleEndian() 
{
  doublebyte sh = 0;
  int sz = sizeof(doublebyte);
  this->Read((char*)&(sh),sz); 
  if (PlatformIsBigEndian)
    {
    sh = swapShort(sh);
    }
  return(sh);
}

quadbyte DICOMFile::ReadQuadByte() 
{
  quadbyte sh;
  int sz = sizeof(quadbyte);
  this->Read((char*)&(sh),sz);
  if (PlatformIsBigEndian) 
    {
    sh = swapLong(sh);
    }
  return(sh);
}

quadbyte DICOMFile::ReadNBytes(int len) 
{
  quadbyte ret = -1;
  switch (len) 
    {
    case 1:
      char ch;
      this->Read(&ch,1);  //from Image
      ret =(quadbyte) ch;
      break;
    case 2:
      ret =(quadbyte) ReadDoubleByte();
      break;
    case 4:
      ret = ReadQuadByte();
      break;
    default:
      std::cerr << "Unable to read " << len << " bytes" << std::endl;
      break;
    }
  return (ret);
}

float DICOMFile::ReadAsciiFloat(int len) 
{
  float ret=0.0;


  char* val = new char[len+1];
  this->Read(val,len);
  val[len] = '\0';

#if 0
  //
  // istrstream destroys the data during formatted input.
  //
  int len2 = static_cast<int> (strlen((char*) val));
  char* val2 = new char[len2];
  strncpy(val2, (char*) val, len2);

  dicom_stream::istrstream data(val2);
  data >> ret;
  delete [] val2;
#else
  sscanf(val,"%e",&ret);
#endif

  std::cout << "Read ASCII float: " << ret << std::endl;

  delete [] val;
  return (ret);
}

int DICOMFile::ReadAsciiInt(int len) 
{
  int ret=0;

  char* val = new char[len+1];
  this->Read(val,len);
  val[len] = '\0';

#if 0
  //
  // istrstream destroys the data during formatted input.
  //
  int len2 = static_cast<int> (strlen((char*) val));
  char* val2 = new char[len2];
  strncpy(val2, (char*) val, len2);

  dicom_stream::istrstream data(val2);
  data >> ret;
  delete [] val2;
#else
  sscanf(val,"%d",&ret);
#endif

  std::cout << "Read ASCII int: " << ret << std::endl;

  delete [] val;
  return (ret);
}

char* DICOMFile::ReadAsciiCharArray(int len) 
{
  if (len <= 0)
    {
    return NULL;
    }
  char* val = new char[len + 1];
  this->Read(val, len);
  val[len] = 0; // NULL terminate.
  return val;
}

#ifdef _MSC_VER
#pragma warning ( pop )
#endif
