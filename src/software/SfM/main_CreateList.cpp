// Copyright (c) 2012, 2013, 2015 Pierre MOULON.

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#include "openMVG/exif_IO/exif_IO_EasyExif.hpp"

#include "openMVG_Samples/sensorWidthDatabase/ParseDatabase.hpp"

#include "openMVG/image/image.hpp"
#include "openMVG/split/split.hpp"

#include "third_party/cmdLine/cmdLine.h"
#include "third_party/stlplus3/filesystemSimplified/file_system.hpp"

#include "third_party/rapidjson/include/rapidjson/document.h"
#include "third_party/rapidjson/include/rapidjson/writer.h"
#include "third_party/rapidjson/include/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/include/rapidjson/filestream.h"
#include "third_party/rapidjson/include/rapidjson/prettywriter.h"

using namespace rapidjson;

#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <string>
#include <vector>

using namespace openMVG;

/// Check that Kmatrix is a string like "f;0;ppx;0;f;ppy;0;0;1"
/// With f,ppx,ppy as valid numerical value
bool checkIntrinsicStringValidity(const std::string & Kmatrix)
{
  std::vector<std::string> vec_str;
  split( Kmatrix, ";", vec_str );
  if (vec_str.size() != 9)  {
    std::cerr << "\n Missing ';' character" << std::endl;
    return false;
  }
  // Check that all K matrix value are valid numbers
  for (size_t i = 0; i < vec_str.size(); ++i) {
    double readvalue = 0.0;
    std::stringstream ss;
    ss.str(vec_str[i]);
    if (! (ss >> readvalue) )  {
      std::cerr << "\n Used an invalid not a number character" << std::endl;
      return false;
    }
  }
  return true;
}

int main(int argc, char **argv)
{
  CmdLine cmd;

  std::string sImageDir,
    sfileDatabase = "",
    sOutputDir = "",
    sKmatrix;

  double focalPixPermm = -1.0;

  cmd.add( make_option('i', sImageDir, "imageDirectory") );
  cmd.add( make_option('d', sfileDatabase, "sensorWidthDatabase") );
  cmd.add( make_option('o', sOutputDir, "outputDirectory") );
  cmd.add( make_option('f', focalPixPermm, "focal") );
  cmd.add( make_option('k', sKmatrix, "intrinsics") );

  try {
      if (argc == 1) throw std::string("Invalid command line parameter.");
      cmd.process(argc, argv);
  } catch(const std::string& s) {
      std::cerr << "Usage: " << argv[0] << '\n'
      << "[-i|--imageDirectory]\n"
      << "[-d|--sensorWidthDatabase]\n"
      << "[-o|--outputDirectory]\n"
      << "[-f|--focal] (pixels)\n"
      << "[-k|--intrinsics] Kmatrix: \"f;0;ppx;0;f;ppy;0;0;1\""
      << std::endl;

      std::cerr << s << std::endl;
      return EXIT_FAILURE;
  }

  std::cout << " You called : " <<std::endl
            << argv[0] << std::endl
            << "--imageDirectory " << sImageDir << std::endl
            << "--sensorWidthDatabase " << sfileDatabase << std::endl
            << "--outputDirectory " << sOutputDir << std::endl
            << "--focal " << focalPixPermm << std::endl
            << "--intrinsics " << sKmatrix << std::endl;

  if ( !stlplus::folder_exists( sImageDir ) )
  {
    std::cerr << "\nThe input directory doesn't exist" << std::endl;
    return EXIT_FAILURE;
  }

  if (sOutputDir.empty())
  {
    std::cerr << "\nInvalid output directory" << std::endl;
    return EXIT_FAILURE;
  }

  if ( !stlplus::folder_exists( sOutputDir ) )
  {
    if ( !stlplus::folder_create( sOutputDir ))
    {
      std::cerr << "\nCannot create output directory" << std::endl;
      return EXIT_FAILURE;
    }
  }

  if (sKmatrix.size() > 0 && !checkIntrinsicStringValidity(sKmatrix) )
  {
    std::cerr << "\nInvalid K matrix input" << std::endl;
    return EXIT_FAILURE;
  }

  if (sKmatrix.size() > 0 && focalPixPermm != -1.0)
  {
    std::cerr << "\nCannot combine -f and -k options" << std::endl;
    return EXIT_FAILURE;
  }

  std::vector<Datasheet> vec_database;
  if (!sfileDatabase.empty())
  {
    if ( !parseDatabase( sfileDatabase, vec_database ) )
    {
      std::cerr << "\nInvalid input database" << std::endl;
      return EXIT_FAILURE;
    }
  }

  std::vector<std::string> vec_image = stlplus::folder_files( sImageDir );
  
  // JSON output
  FileStream jsonOut( fopen(stlplus::create_filespec( sOutputDir, "imageParams.json" ).c_str(),"w") );
  PrettyWriter<FileStream> jsonWriter(jsonOut);

  // Start an array of images with their paramaters
  jsonWriter.StartArray();

  std::sort(vec_image.begin(), vec_image.end());
  for ( std::vector<std::string>::const_iterator iter_image = vec_image.begin();
    iter_image != vec_image.end();
    iter_image++ )
  {
    // Read meta data to fill width height and focalPixPermm
    const std::string sImageFilename = stlplus::create_filespec( sImageDir, *iter_image );
    
    // Test if the image format is supported:
    if (openMVG::GetFormat(sImageFilename.c_str()) == openMVG::Unknown)
      continue; // image cannot be opened

    size_t width = -1, height = -1;
    Image<unsigned char> image;
    if (openMVG::ReadImage( sImageFilename.c_str(), &image))  {
      width = image.Width();
      height = image.Height();
    }
    else
      continue; // image cannot be read

    jsonWriter.StartObject();
    jsonWriter.String("filename");
    jsonWriter.String( (*iter_image).c_str() );
    jsonWriter.String("width");
    jsonWriter.Uint(width);
    jsonWriter.String("height");
    jsonWriter.Uint(height);

    if(focalPixPermm != -1)
    {
      jsonWriter.String("focalLength");
      jsonWriter.Double(focalPixPermm);
    }
    if(sKmatrix.size() > 0)
    {
      jsonWriter.String("sKmatrix");
      jsonWriter.String(sKmatrix.c_str());
    }
 
    // Read EXIF data
    std::auto_ptr<Exif_IO> exifReader (new Exif_IO_EasyExif() );
    exifReader->open( sImageFilename );

    if ( exifReader->doesHaveExifInfo() )
    {
      const std::string sCamName = exifReader->getBrand();
      const std::string sCamModel = exifReader->getModel();
      jsonWriter.String("camera");
      jsonWriter.StartObject();
      jsonWriter.String("name");
      jsonWriter.String(sCamName.c_str());
      jsonWriter.String("model");
      jsonWriter.String(sCamModel.c_str());
      jsonWriter.EndObject();

      if(focalPixPermm == -1)
      {
        // Warning if the focal length is equal to 0
        if (exifReader->getFocal() == 0.0f)
        {
          std::cerr << stlplus::basename_part(sImageFilename) << ": Focal length is missing from EXIF data." << std::endl;
        }
        else // Lookup focal length in the camera database
        {
          Datasheet datasheet;
          if ( getInfo( sCamName, sCamModel, vec_database, datasheet ))
          {
            // The camera model was found in the database so we can compute it's approximated focal length
            const double ccdw = datasheet._sensorSize;
            const double focal = std::max ( width, height ) * exifReader->getFocal() / ccdw;

            jsonWriter.String("focalLength");
            jsonWriter.Double(focal);
          }
          else
          {
            std::cout << stlplus::basename_part(sImageFilename) << ": Camera \"" 
              << sCamName << "\" model \"" << sCamModel << "\" doesn't exist in the database" << std::endl
              << "Please consider add your camera model and sensor width in the database." << std::endl;
          }
        }
      }
    }

    jsonWriter.EndObject();
  }
  jsonWriter.EndArray();
  return EXIT_SUCCESS;
}
