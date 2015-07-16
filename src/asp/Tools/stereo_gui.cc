// __BEGIN_LICENSE__
//  Copyright (c) 2009-2013, United States Government as represented by the
//  Administrator of the National Aeronautics and Space Administration. All
//  rights reserved.
//
//  The NGT platform is licensed under the Apache License, Version 2.0 (the
//  "License"); you may not use this file except in compliance with the
//  License. You may obtain a copy of the License at
//  http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
// __END_LICENSE__


/// \file stereo_gui.cc
///

#include <QApplication>
#include <QWidget>

#include <vw/InterestPoint.h>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <vw/Stereo/PreFilter.h>
#include <vw/Stereo/CorrelationView.h>
#include <vw/Stereo/CostFunctions.h>
#include <vw/Stereo/DisparityMap.h>
#include <asp/Tools/stereo.h>
#include <asp/Core/DemDisparity.h>
#include <asp/Core/LocalHomography.h>
#include <asp/GUI/MainWindow.h>

using namespace vw;
using namespace vw::stereo;
using namespace asp;
using namespace std;

// Allows FileIO to correctly read/write unusual pixel types
namespace vw {
  template<> struct PixelFormatID<Vector3>   { static const PixelFormatEnum value = VW_PIXEL_GENERIC_3_CHANNEL; };
}

int main(int argc, char* argv[]) {

  try {

    stereo_register_sessions();

    bool verbose = false;
    vector<Options> opt_vec;
    string output_prefix;
    std::vector<std::string> images;
    try {
      // See if we passed all the correct options for stereo
      asp::parse_multiview(argc, argv, GUIDescription(),
                           verbose, output_prefix, opt_vec);
      // Extract the images from the options, not the cameras though.
      for (size_t i = 0; i < opt_vec.size(); i++) {
        if (i == 0) images.push_back(opt_vec[0].in_file1);
        images.push_back(opt_vec[0].in_file2);
      }

    }catch (std::exception& e){

      // The tool was invoked as an image viewer.
      for (int i = 1; i < argc; i++) {
        std::string image = argv[i];

        // See if this image is valid
        if ( image != "" && image[0] != '-') {
          if (!fs::exists(image)) {
            vw_throw(ArgumentErr() << "Invalid input arguments. Either stereo options "
                     << "were not provided, or, if invoked as a viewer only, "
                     << "not all input images are present.\n" );
          }
          images.push_back(image);
        }
      }

      // Presumably the tool was invoked with no options.
      if (images.empty())
        vw_throw(ArgumentErr() << e.what() << "\n");

    }

    std::cout << "---out prefix is " << output_prefix << std::endl;
    std::cout << "--window size is " << stereo_settings().window_size << std::endl;


    for (size_t i = 0; i < images.size(); i++) {
      std::cout << "image is " << images[i] << std::endl;
    }

    // TODO: Expose these as options and fix the current broken
    // implementation.
    bool ignore_georef = true;
    bool hillshade = false;

    // Start up the Qt GUI
    QApplication app(argc, argv);
    vw::gui::MainWindow main_window(images, output_prefix,
                                    stereo_settings().window_size,
                                    ignore_georef, hillshade, argc, argv);
    main_window.show();
    app.exec();

  } ASP_STANDARD_CATCHES;

  return 0;
}