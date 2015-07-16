// __BEGIN_LICENSE__
//  Copyright (c) 2006-2013, United States Government as represented by the
//  Administrator of the National Aeronautics and Space Administration. All
//  rights reserved.
//
//  The NASA Vision Workbench is licensed under the Apache License,
//  Version 2.0 (the "License"); you may not use this file except in
//  compliance with the License. You may obtain a copy of the License at
//  http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
// __END_LICENSE__


/// \file stereo_gui_MainWindow.cc
///
/// The Vision Workbench image viewer main window class.
///
#include <QtGui>
#include <asp/GUI/MainWindow.h>
#include <asp/GUI/MainWidget.h>
#include <asp/GUI/Utils.h>
using namespace vw::gui;

#include <vw/config.h>
#include <vw/Image/MaskViews.h>
#include <vw/FileIO/DiskImageView.h>
#include <vw/FileIO/DiskImageResource.h>
#include <vw/Image/Statistics.h>
#include <vw/Image/PixelMask.h>
#include <boost/filesystem/path.hpp>

#include <sstream>
namespace po = boost::program_options;

MainWindow::MainWindow(std::vector<std::string> const& images,
                       std::string const& output_prefix,
                       vw::Vector2i const& window_size,
                       bool ignore_georef, bool hillshade,
                       int argc,  char ** argv) :
  m_images(images), m_output_prefix(output_prefix),
  m_widRatio(0.3), m_main_widget(NULL),
  m_chooseFiles(NULL), m_argc(argc), m_argv(argv) {

  resize(window_size[0], window_size[1]);

  // Set the window title and add tabs
  std::string window_title = "Stereo GUI";
  this->setWindowTitle(window_title.c_str());

  // Set up the basic layout of the window and its menus.
  create_menus();

  //  if (images.size() > 1){

  // Split the window into two, with the sidebar on the left

  QWidget * centralFrame;
  centralFrame = new QWidget(this);
  setCentralWidget(centralFrame);

  QSplitter *splitter = new QSplitter(centralFrame);
#if 0
  m_chooseFiles = new chooseFilesDlg(this);
  m_chooseFiles->setMaximumSize(int(m_widRatio*size().width()), size().height());
  m_main_widget = new MainWidget(centralFrame, images, m_chooseFiles,
                                 ignore_georef, hillshade);
  splitter->addWidget(m_chooseFiles);
  splitter->addWidget(m_main_widget);
#else
  // for doing stereo
  for (size_t i = 0; i < images.size(); i++) {
    // TODO: Invoke here asp's handle_arguments().
    // For now we just display all images that are valid.
    bool is_image = true;
    try {
      DiskImageView<float> img(images[i]);
    }catch(...){
      is_image = false;
    }
    if (!is_image) continue;
    std::vector<std::string> local_images;
    local_images.push_back(images[i]);
    MainWidget * widget = new MainWidget(centralFrame, local_images,
                                         m_chooseFiles,
                                         ignore_georef, hillshade);

    // TODO: When should we de-allocate m_widgets and m_main_widget?
    m_widgets.push_back(widget);
    splitter->addWidget(widget);
  }
#endif
  QGridLayout *layout = new QGridLayout(centralFrame);
  layout->addWidget (splitter, 0, 0, 0);
  centralFrame->setLayout (layout);
  //   }else{
  //     // Set up MainWidget
  //     m_main_widget = new MainWidget(this, images, NULL, ignore_georef, hillshade);
  //     setCentralWidget(m_main_widget);
  //   }

}

//********************************************************************
//                      MAIN WINDOW SETUP
//********************************************************************

void MainWindow::create_menus() {

  QMenuBar* menu = menuBar();

  // Exit or Quit
  m_exit_action = new QAction(tr("Exit"), this);
  m_exit_action->setShortcut(tr("Q"));
  m_exit_action->setStatusTip(tr("Exit the application"));
  connect(m_exit_action, SIGNAL(triggered()), this, SLOT(forceQuit()));

  // Run stereo
  m_run_stereo_action = new QAction(tr("Run stereo"), this);
  m_run_stereo_action->setStatusTip(tr("Run stereo on selected clips."));
  connect(m_run_stereo_action, SIGNAL(triggered()), this, SLOT(run_stereo()));
  m_run_stereo_action->setShortcut(tr("R"));

  // Size to fit
  m_size_to_fit_action = new QAction(tr("Size to fit"), this);
  m_size_to_fit_action->setStatusTip(tr("Change the view to encompass the images."));
  connect(m_size_to_fit_action, SIGNAL(triggered()), this, SLOT(size_to_fit()));
  m_size_to_fit_action->setShortcut(tr("F"));

  // The About Box
  m_about_action = new QAction(tr("About stereo_gui"), this);
  m_about_action->setStatusTip(tr("Show the stereo_gui about box."));
  connect(m_about_action, SIGNAL(triggered()), this, SLOT(about()));

  // File menu
  m_file_menu = menu->addMenu(tr("&File"));
  m_file_menu->addAction(m_exit_action);

  // Run menu
  m_file_menu = menu->addMenu(tr("&Run"));
  m_file_menu->addAction(m_run_stereo_action);

  // View menu
  menu->addSeparator();
  m_view_menu = menu->addMenu(tr("&View"));
  m_view_menu->addAction(m_size_to_fit_action);

  // Help menu
  menu->addSeparator();
  m_help_menu = menu->addMenu(tr("&Help"));
  m_help_menu->addAction(m_about_action);
}

void MainWindow::resizeEvent(QResizeEvent *){
  if (m_chooseFiles)
    m_chooseFiles->setMaximumSize(int(m_widRatio*size().width()), size().height());
}

void MainWindow::closeEvent(QCloseEvent *){
  forceQuit();
}

void MainWindow::forceQuit(){
  exit(0); // A fix for an older buggy version of Qt
}

void MainWindow::size_to_fit(){
  if (m_main_widget)
    m_main_widget->size_to_fit();
  for (size_t i = 0; i < m_widgets.size(); i++) {
    if (m_widgets[i])
      m_widgets[i]->size_to_fit();
  }
}

void MainWindow::run_stereo(){

  if (m_widgets.size() >= 2) {
    QRect left_win = m_widgets[0]->get_crop_win();
    QRect right_win = m_widgets[1]->get_crop_win();

    int left_x = left_win.x();
    int left_y = left_win.y();
    int left_wx = left_win.width();
    int left_wy = left_win.height();

    int right_x = right_win.x();
    int right_y = right_win.y();
    int right_wx = right_win.width();
    int right_wy = right_win.height();

    // Run stereo
    std::string run_cmd = "stereo ";
    for (int i = 1; i < m_argc; i++) {
      run_cmd += std::string(m_argv[i]) + " ";
    }
    std::ostringstream os;
    os << "--left-image-crop-win " << left_x << " " << left_y << " "
       << left_wx << " " << left_wy << " ";
    os << "--right-image-crop-win " << right_x << " " << right_y << " "
       << right_wx << " " << right_wy << " ";
    run_cmd += os.str();
    std::cout << "Running: " << run_cmd << std::endl;
    system(run_cmd.c_str());
    QMessageBox::about(this, tr("Error"), tr("Done running stereo"));

  } else {
    QMessageBox::about(this, tr("Error"), tr("Not ready to run stereo"));
  }
}

void MainWindow::about() {
  std::ostringstream about_text;
  about_text << "<h3>stereo_gui</h3>"
             << "<p>Copyright &copy; 2015 NASA Ames Research Center. See the manual for documentation.</p>";
  QMessageBox::about(this, tr("About stereo_gui"),
                     tr(about_text.str().c_str()));

}

void MainWindow::keyPressEvent(QKeyEvent *event) {

  std::ostringstream s;

  switch (event->key()) {
  case Qt::Key_Escape:  // Quit
    close();
    break;
  }
}