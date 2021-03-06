#include "main_view.h"
#include "ui_main_view.h"

#include <vector>
#include <QGraphicsItemGroup>
#include <QMessageBox>
#include <QDebug>
#include <QPen>
#include <QFileDialog>
#include <QIntValidator>
#include <QGraphicsView>

#include "controller/main_controller.h"
#include "config_singleton.h"
#include "utils/geom_utils.h"

using namespace std;
using namespace viewpkg;

/*
 * constructors
 */

MainView::MainView(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainView)
{
    ui->setupUi(this);

    ui->graphicsView->setScene(&scene);
    ui->graphicsView->setMainView(this);
    ui->graphicsView->setMouseTracking(true);
    zoom = ui->graphicsView->getZoom();

    ui->progressBar->setVisible(false);

    ui->graphicsView->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    ui->graphicsView->setCacheMode(QGraphicsView::CacheBackground);

    QObject::connect(ui->calculate_btn, SIGNAL(clicked(bool)), &scene.getFirstTrajectory(), SLOT(cleanSelection()));
    QObject::connect(ui->calculate_btn, SIGNAL(clicked(bool)), &scene.getSecondTrajectory(), SLOT(cleanSelection()));

    QObject::connect(&scene.getFirstTrajectory(), SIGNAL(frameDoubleClicked(int,bool)), this, SLOT(onFirstTrajectoryDoubleClicked(int,bool)));
    QObject::connect(&scene.getSecondTrajectory(), SIGNAL(frameDoubleClicked(int,bool)), this, SLOT(onSecondTrajectoryDoubleClicked(int,bool)));

    scene.getFirstTrajectory().setOrientationVisible(ui->is_orientation_show_chk->isChecked());
    scene.getSecondTrajectory().setOrientationVisible(ui->is_orientation_show_chk->isChecked());

    scene.getFirstTrajectory().setDirectionVisible(ui->is_direction_show_chk->isChecked());
    scene.getSecondTrajectory().setDirectionVisible(ui->is_direction_show_chk->isChecked());

    scene.getFirstTrajectory().setKeyPointsVisible(ui->is_key_point_show_chk->isChecked());
    scene.getSecondTrajectory().setKeyPointsVisible(ui->is_key_point_show_chk->isChecked());

    scene.getMainMap().setVisible(ui->is_map_show_check->isChecked());

    scene.getMatches().setVisible(ui->is_matches_show_check->isChecked());

    on_is_recovery_show_check_toggled(ui->is_recovery_show_check->isChecked());

    //intially update shift
    on_shift_second_trj_group_toggled(ui->shift_second_trj_group->isChecked());

    scene.setMainView(this);
}

MainView::~MainView()
{
    delete ui;
}

/*
 * main public interface
 */

void MainView::setController(shared_ptr<controllerpkg::MainController> controller)
{
    this->controller = controller;
}

void MainView::setFirstTrajectory(const vector<QPixmap> &imgs, const vector<QPointF> center_coords_px,
                                  const vector<double> &angles, const vector<double> &meters_per_pixels,
                                  const vector<double> &qualities)
{

    scene.getFirstTrajectory().clear();
    for (size_t i = 0; i < imgs.size(); i++)
    {
        scene.getFirstTrajectory().pushBackFrame(imgs[i], center_coords_px[i], angles[i], meters_per_pixels[i], qualities[i]);
    }
}

void MainView::setSecondTrajectory(const vector<QPixmap> &imgs, const vector<QPointF> center_coords_px,
                                   const vector<double> &angles, const vector<double> &meters_per_pixels,
                                   const vector<double> &qualities)
{
    scene.getSecondTrajectory().clear();
    for (size_t i = 0; i < imgs.size(); i++)
    {
        scene.getSecondTrajectory().pushBackFrame(imgs[i], center_coords_px[i], angles[i], meters_per_pixels[i], qualities[i]);
    }
}

void MainView::setGhostRecovery(QPointF pos, double width, double height, double angle)
{
  scene.getGhostRecovery().setRect(pos.x() - width/2., pos.y()-height/2.,
                                   width, height);
  scene.getGhostRecovery().setTransformOriginPoint(pos);
  scene.getGhostRecovery().setRotation(angle);

  scene.getGhostOrientation().setCenter(pos);
  scene.getGhostOrientation().setAxisLength(min(width, height)/4.);
  scene.getGhostOrientation().setTransformOriginPoint(pos);
  scene.getGhostOrientation().setRotation(angle);
}

void MainView::clearGhostRecovery()
{
  scene.getGhostRecovery().setRect(0, 0, 0, 0);
  scene.getGhostOrientation().setCenter(QPointF(0, 0));
  scene.getGhostOrientation().setAxisLength(0);
}

void MainView::setMainMap(QPixmap map, double meter_per_pixel)
{
    scene.getMainMap().setMapItem(map);
    scene.getMainMap().setScale(meter_per_pixel);
}

void MainView::setAlgorithms(const vector<QString> &algorithms_names)
{
  ui->algorithm_combo->clear();
  for (const auto &name: algorithms_names)
  {
      ui->algorithm_combo->addItem(name);
  }
}

void MainView::setDetectors(const vector<QString> &detectors_names)
{
    ui->detector_combo->clear();
    for (const auto &name: detectors_names)
    {
        ui->detector_combo->addItem(name);
    }
}

void MainView::setDescriptors(const vector<QString> &descriptors_names)
{
    ui->descriptor_combo->clear();
    for (const auto &name: descriptors_names)
    {
        ui->descriptor_combo->addItem(name);
    }
}

void MainView::showException(QString what)
{
    QMessageBox::critical(this, "Error", what, QMessageBox::Ok, QMessageBox::Default);
}

void MainView::setTrajectoryPath(int trj_num, QString path)
{
  if (trj_num == 0)
  {
    ui->first_traj_edit->setText(path);
  }
  else if (trj_num == 1)
  {
    ui->second_traj_edit->setText(path);
  }
}

void MainView::setProgressBarTask(QString name, bool reset)
{
  ui->progressBar->setFormat(name + " %p%");
  if (reset)
  {
    ui->progressBar->setValue(ui->progressBar->minimum());
  }
}

void MainView::setProgressBarValue(int value, int maximum, int minimum)
{
  ui->progressBar->setMaximum(maximum);
  ui->progressBar->setMinimum(minimum);
  ui->progressBar->setValue(value);
  QApplication::processEvents();
}

void MainView::setEnabledDataCalculating(bool isEnabled, int trj_num)
{
  if (trj_num == -1)
  {
    ui->calc_first_trj_btn->setEnabled(isEnabled);
    ui->calc_sec_trj_btn->setEnabled(isEnabled);
  }
  else if (trj_num == 0)
  {
    ui->calc_first_trj_btn->setEnabled(isEnabled);
  }
  else if (trj_num == 1)
  {
    ui->calc_sec_trj_btn->setEnabled(isEnabled);
  }

  bool calc_both_enabled = ui->calc_first_trj_btn->isEnabled() &&
                           ui->calc_sec_trj_btn->isEnabled();
  ui->calculate_btn->setEnabled(calc_both_enabled);

  bool settings_enabled = ui->calc_first_trj_btn->isEnabled() ||
                          ui->calc_sec_trj_btn->isEnabled();
  ui->calculation_settings_group->setEnabled(settings_enabled);
}

void MainView::setEnabledDataManipulating(bool isEnabled)
{
  ui->maniplations_group->setEnabled(isEnabled);
}

/*
 * private slots
 * buttons
 */

void viewpkg::MainView::on_load_ini_btn_clicked()
{
    string ini_filename = ui->ini_edit->text().trimmed().toStdString();
    controller->loadIni(ini_filename);

    ConfigSingleton &cfg = ConfigSingleton::getInstance();

    ui->first_traj_edit->setText(
          QString::fromStdString(cfg.getPathToTrajectoryCsv(0)));
    ui->second_traj_edit->setText(
          QString::fromStdString(cfg.getPathToTrajectoryCsv(1)));
}

void viewpkg::MainView::on_calculate_btn_clicked()
{
//    int detector_idx = ui->detector_combo->currentIndex();
//    int descriptor_idx = ui->descriptor_combo->currentIndex();

//    controller->loadOrCalculateModel(0, detector_idx, descriptor_idx);
//    controller->loadOrCalculateModel(1, detector_idx, descriptor_idx);
  on_calc_first_trj_btn_clicked();
  on_calc_sec_trj_btn_clicked();
}


void viewpkg::MainView::on_calc_first_trj_btn_clicked()
{
  int algorithm_idx = ui->algorithm_combo->currentIndex();
  int detector_idx = ui->detector_combo->currentIndex();
  int descriptor_idx = ui->descriptor_combo->currentIndex();
  size_t max_key_points_per_frame = ui->max_key_points_spin->value();

  controller->loadOrCalculateModel(0, algorithm_idx,
                                   detector_idx, descriptor_idx,
                                   max_key_points_per_frame);
}

void viewpkg::MainView::on_calc_sec_trj_btn_clicked()
{
  int algorithm_idx = ui->algorithm_combo->currentIndex();
  int detector_idx = ui->detector_combo->currentIndex();
  int descriptor_idx = ui->descriptor_combo->currentIndex();
  size_t max_key_points_per_frame = ui->max_key_points_spin->value();

  controller->loadOrCalculateModel(1, algorithm_idx,
                                   detector_idx, descriptor_idx,
                                   max_key_points_per_frame);
}

void viewpkg::MainView::on_match_btn_clicked()
{
    controller->calculateMatches();
}

void viewpkg::MainView::on_recover_trajectory_btn_clicked()
{
    controller->recoverTrajectory(ui->score_thres_spinbox->value());
}

void viewpkg::MainView::on_load_first_trj_btn_clicked()
{
    controller->loadTrajectory(0, ui->first_traj_edit->text().toStdString());
}

void viewpkg::MainView::on_load_sec_trj_btn_clicked()
{
    controller->loadTrajectory(1, ui->second_traj_edit->text().toStdString());
}

/*
 * checkboxes
 */

void viewpkg::MainView::on_is_trajectory_show_chk_toggled(bool checked)
{
    scene.getFirstTrajectory().setTrajectoryVisible(checked);
    scene.getSecondTrajectory().setTrajectoryVisible(checked);
}

void viewpkg::MainView::on_is_orientation_show_chk_toggled(bool checked)
{
    scene.getFirstTrajectory().setOrientationVisible(checked);
    scene.getSecondTrajectory().setOrientationVisible(checked);
}

void viewpkg::MainView::on_is_direction_show_chk_toggled(bool checked)
{
    scene.getFirstTrajectory().setDirectionVisible(checked);
    scene.getSecondTrajectory().setDirectionVisible(checked);
}

void viewpkg::MainView::on_is_key_point_show_chk_toggled(bool checked)
{
    scene.getFirstTrajectory().setKeyPointsVisible(checked);
    scene.getSecondTrajectory().setKeyPointsVisible(checked);
}

void viewpkg::MainView::on_is_matches_show_check_toggled(bool checked)
{
    scene.getMatches().setVisible(checked);
}

void viewpkg::MainView::on_is_map_show_check_toggled(bool checked)
{
    scene.getMainMap().setVisible(checked);
}

void viewpkg::MainView::on_is_recovery_show_check_toggled(bool checked)
{
    scene.getGhostRecovery().setVisible(checked);
    scene.getGhostOrientation().setVisible(checked);
}

void viewpkg::MainView::on_filter_recovered_by_score_check_toggled(bool checked)
{
    controller->filterByScore(checked);
}

/*
 * frame selection
 */

void MainView::onFirstTrajectoryDoubleClicked(int frame_num, bool isSelected)
{
    if (isSelected)
    {
        scene.getSecondTrajectory().cleanSelection();
        controller->selectedFrame(0, frame_num);
    }
    else
    {
        controller->unselectedFrame(0, frame_num);
    }
}

void MainView::onSecondTrajectoryDoubleClicked(int frame_num, bool isSelected)
{
    if (isSelected)
    {
        scene.getFirstTrajectory().cleanSelection();
        controller->selectedFrame(1, frame_num);
    }
    else
    {
        controller->unselectedFrame(1, frame_num);
    }
}

/*
 * interface for trajectory shifting
 */
void viewpkg::MainView::on_trj2_shift_x_spin_valueChanged(int shift_x)
{
    updateShift(shift_x, ui->trj2_shift_y_spin->value());
}

void viewpkg::MainView::on_trj2_shift_y_spin_valueChanged(int shift_y)
{
    updateShift(ui->trj2_shift_x_spin->value(), shift_y);
}

void viewpkg::MainView::on_shift_second_trj_group_toggled(bool isChecked)
{
    if (isChecked)
    {
        updateShift(ui->trj2_shift_x_spin->value(),
                    ui->trj2_shift_y_spin->value());
    }
    else
    {
        updateShift(0, 0);
    }
}

void MainView::updateShift(int shift_x, int shift_y)
{
    QPointF shift(shift_x, shift_y);

    scene.getSecondTrajectory().setPos(shift);
    scene.getMatches().setShift(shift);
}

/*
 * interface for status bar
 */

void MainView::setMouseScenePosition(QPointF pos)
{
    mouse_scene_pos_m = pos;
    updateStatusBar();
}

void MainView::setZoom(double zoom)
{
    this->zoom = zoom;
    updateStatusBar();
}

void MainView::updateStatusBar()
{
    QString position_str = QString("Position: (%1, %2) meters").arg(QString::number(mouse_scene_pos_m.x(), 'f', 3), QString::number(mouse_scene_pos_m.y(), 'f', 3));
    QString zoom_str = QString("Zoom: %1%").arg(QString::number(zoom*100));
    QString meters_str = QString("Meters per pixel: %1").arg(QString::number(1. / zoom));
    ui->statusBar->showMessage( QString("%3 | %2 | %1").arg(position_str, meters_str, zoom_str) );
}

void viewpkg::MainView::on_open_ini_file_btn_clicked()
{
  QString path_to_config = QFileDialog::getOpenFileName(this, "Path to config");

  if (path_to_config != "")
  {
    ui->ini_edit->setText(path_to_config);
  }
}

void viewpkg::MainView::on_open_first_trj_btn_clicked()
{
  QString path_to_first_trj = QFileDialog::getOpenFileName(this,
                                                           "Path to first trj");
  if (path_to_first_trj != "")
  {
    ui->first_traj_edit->setText(path_to_first_trj);
  }
}

void viewpkg::MainView::on_open_sec_trj_btn_clicked()
{
  QString path_to_second_trj = QFileDialog::getOpenFileName(this,
                                                          "Path to second trj");
  if (path_to_second_trj != "")
  {
    ui->second_traj_edit->setText(path_to_second_trj);
  }
}

void viewpkg::MainView::on_progressBar_valueChanged(int value)
{
  if (value == ui->progressBar->minimum())
  {
    ui->progressBar->setVisible(true);
  }
  else if (value >= ui->progressBar->maximum())
  {
    ui->progressBar->setVisible(false);
  }
}

