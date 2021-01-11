/**
 * Copyright (C) 2015 by Liangliang Nan (liangliang.nan@gmail.com)
 * https://3d.bk.tudelft.nl/liangliang/
 *
 * This file is part of Easy3D. If it is useful in your research/work,
 * I would be grateful if you show your appreciation by citing it:
 * ------------------------------------------------------------------
 *      Liangliang Nan.
 *      Easy3D: a lightweight, easy-to-use, and efficient C++
 *      library for processing and rendering 3D data. 2018.
 * ------------------------------------------------------------------
 * Easy3D is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 3
 * as published by the Free Software Foundation.
 *
 * Easy3D is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "dialog_walk_through.h"
#include "paint_canvas.h"
#include "main_window.h"

#include <easy3d/core/model.h>
#include <easy3d/renderer/walk_throuth.h>
#include <easy3d/renderer/key_frame_interpolator.h>
#include <easy3d/renderer/manipulated_camera_frame.h>
#include <easy3d/renderer/camera.h>
#include <easy3d/util/logging.h>
#include <easy3d/util/file_system.h>
#include <easy3d/util/stop_watch.h>

#include <QFileDialog>
#include <QMessageBox>
#include <QButtonGroup>


using namespace easy3d;

DialogWalkThrough::DialogWalkThrough(MainWindow *window)
	: Dialog(window)
{
	setupUi(this);

	spinBoxFPS->setValue(interpolator()->frame_rate());
	doubleSpinBoxInterpolationSpeed->setValue(interpolator()->interpolation_speed());

	connect(doubleSpinBoxCharacterHeightFactor, SIGNAL(valueChanged(double)), this, SLOT(setCharacterHeightFactor(double)));
	connect(doubleSpinBoxCharacterDistanceFactor, SIGNAL(valueChanged(double)), this, SLOT(setCharacterDistanceFactor(double)));

    connect(spinBoxFPS, SIGNAL(valueChanged(int)), this, SLOT(setFrameRate(int)));
    connect(doubleSpinBoxInterpolationSpeed, SIGNAL(valueChanged(double)), this, SLOT(setInterpolationSpeed(double)));

    connect(importCameraPathButton, SIGNAL(clicked()), this, SLOT(importCameraPathFromFile()));
    connect(exportCameraPathButton, SIGNAL(clicked()), this, SLOT(exportCameraPathToFile()));

    connect(checkBoxShowCameraPath, SIGNAL(toggled(bool)), this, SLOT(showCameraPath(bool)));

    connect(radioButtonWalkingMode, SIGNAL(toggled(bool)), this, SLOT(setWalkingMode(bool)));

	connect(previousKeyframeButton, SIGNAL(clicked()), this, SLOT(goToPreviousKeyframe()));
	connect(nextKeyframeButton, SIGNAL(clicked()), this, SLOT(goToNextKeyframe()));
    connect(removeLastKeyframeButton, SIGNAL(clicked()), this, SLOT(removeLastKeyframe()));

    connect(horizontalSliderPreview, SIGNAL(valueChanged(int)), this, SLOT(goToKeyframe(int)));

    connect(clearCameraPathButton, SIGNAL(clicked()), this, SLOT(clearPath()));
    connect(previewButton, SIGNAL(toggled(bool)), this, SLOT(preview(bool)));
    connect(recordButton, SIGNAL(clicked()), this, SLOT(record()));

    connect(browseButton, SIGNAL(clicked()), this, SLOT(browse()));

    easy3d::connect(&walkThrough()->path_modified, this, &DialogWalkThrough::numKeyramesChanged);

    QButtonGroup* group = new QButtonGroup(this);
    group->addButton(radioButtonFreeMode);
    group->addButton(radioButtonWalkingMode);
    radioButtonWalkingMode->setChecked(true);
}


DialogWalkThrough::~DialogWalkThrough()
{
}


void DialogWalkThrough::numKeyramesChanged() {
    disconnect(horizontalSliderPreview, SIGNAL(valueChanged(int)), this, SLOT(goToKeyframe(int)));
    int num = interpolator()->number_of_keyframes();
    if (num == 1) // range is [0, 0]
        horizontalSliderPreview->setEnabled(false);
    else {
        horizontalSliderPreview->setEnabled(true);
        horizontalSliderPreview->setRange(0, std::max(0, num - 1));
    }
    int pos = walkThrough()->current_keyframe_index();
    if (pos >= 0)
        horizontalSliderPreview->setValue(pos);
    connect(horizontalSliderPreview, SIGNAL(valueChanged(int)), this, SLOT(goToKeyframe(int)));
}


easy3d::WalkThrough* DialogWalkThrough::walkThrough() {
    return viewer_->walkThrough();
}


easy3d::KeyFrameInterpolator* DialogWalkThrough::interpolator() {
    return walkThrough()->interpolator();
}


void DialogWalkThrough::showEvent(QShowEvent* e) {
    viewer_->camera()->setZClippingCoefficient(5.0f);
    viewer_->camera()->setZNearCoefficient(0.0001);

    if (radioButtonWalkingMode->isChecked())
        walkThrough()->set_status(easy3d::WalkThrough::WALKING_MODE);
    else
        walkThrough()->set_status(easy3d::WalkThrough::FREE_MODE);

	doubleSpinBoxCharacterHeightFactor->setValue(walkThrough()->height_factor());
	doubleSpinBoxCharacterDistanceFactor->setValue(walkThrough()->third_person_forward_factor());

#ifdef HAS_FFMPEG
    std::string name = "./video.mp4";
    if (viewer_->currentModel())
        name = file_system::replace_extension(viewer_->currentModel()->name(), "mp4");
#else
    std::string name = "./video.png";
    if (viewer_->currentModel())
        name = file_system::replace_extension(viewer_->currentModel()->name(), "png");
#endif

    lineEditOutputFile->setText(QString::fromStdString(name));
	QDialog::showEvent(e);
}


void DialogWalkThrough::closeEvent(QCloseEvent* e) {
    viewer_->camera()->setZClippingCoefficient(std::sqrt(3.0f));
    viewer_->camera()->setZNearCoefficient(0.005);

    walkThrough()->set_status(easy3d::WalkThrough::STOPPED);
    QDialog::closeEvent(e);
	viewer_->update();
}


void DialogWalkThrough::setCharacterHeightFactor(double h) {
    walkThrough()->set_height_factor(h);
    DLOG(WARNING) << "TODO: allow to modify the last keyframe (camera position and orientation) here" << std::endl;
    viewer_->update();
}


void DialogWalkThrough::setCharacterDistanceFactor(double d) {
    walkThrough()->set_third_person_forward_factor(d);
    DLOG(WARNING) << "TODO: allow to modify the last keyframe (camera position and orientation) here" << std::endl;
    viewer_->update();
}


void DialogWalkThrough::setInterpolationSpeed(double s) {
    interpolator()->set_interpolation_speed(s);
    viewer_->update();
}


void DialogWalkThrough::setFrameRate(int fps) {
    interpolator()->set_frame_rate(fps);
    viewer_->update();
}


void DialogWalkThrough::setWalkingMode(bool b) {
    labelCharacterHeight->setEnabled(b);
    labelCharacterDistanceToEye->setEnabled(b);
    doubleSpinBoxCharacterHeightFactor->setEnabled(b);
    doubleSpinBoxCharacterDistanceFactor->setEnabled(b);

    if (b)
        walkThrough()->set_status(easy3d::WalkThrough::WALKING_MODE);
    else
        walkThrough()->set_status(easy3d::WalkThrough::FREE_MODE);
}


void DialogWalkThrough::goToPreviousKeyframe()
{
    const int pos = walkThrough()->current_keyframe_index();
    if (pos <= 0)  // if not started (or at the 1st keyframe), move to the start view point
        walkThrough()->move_to(0);
    else
        walkThrough()->move_to(pos - 1);
    viewer_->update();
    LOG(INFO) << "moved to keyframe " << walkThrough()->current_keyframe_index();
}


void DialogWalkThrough::goToNextKeyframe()
{
    int pos = walkThrough()->current_keyframe_index();
    if (pos >= interpolator()->number_of_keyframes() - 1)  // if already at the end, move to the last view point
        walkThrough()->move_to(interpolator()->number_of_keyframes() - 1);
    else
        walkThrough()->move_to(pos + 1);
    viewer_->update();
    LOG(INFO) << "moved to keyframe " << walkThrough()->current_keyframe_index();
}


void DialogWalkThrough::removeLastKeyframe() {
    if (interpolator()->number_of_keyframes() == 0)
        LOG(WARNING) << "no keyframe can be removed (empty path)";
    else {
        walkThrough()->delete_last_keyframe();
        int index = walkThrough()->current_keyframe_index();
        viewer_->update();

        if (index == -1)
            LOG(WARNING) << "no keyframe can be removed (empty path)";
        else
            LOG(INFO) << "last keyframe removed (current keyframe: " << index << ")";
    }
}


void DialogWalkThrough::goToKeyframe(int p) {
    walkThrough()->move_to(p, false);
    viewer_->update();
}


void DialogWalkThrough::clearPath() {
    if (interpolator()->number_of_keyframes() == 0 && interpolator()->number_of_keyframes() == 0) {
        LOG(WARNING) << "nothing to clear (empty path)";
        return;
    }

    if (QMessageBox::warning(
            viewer_, "Please confirm!",
            "This will delete the previously defined animation path, which cannot be undone.\n"
            "You may export the path to a file before you delete it."
            "\nContinue to delete?", QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
    {
        walkThrough()->delete_path();
        viewer_->update();
	}
}


void DialogWalkThrough::browse() {
    std::string suggested_name;
    if (viewer_->currentModel())
        suggested_name = file_system::replace_extension(viewer_->currentModel()->name(), "mp4");
    const QString fileName = QFileDialog::getSaveFileName(this,
                                                          tr("Choose a file name"), QString::fromStdString(suggested_name),
                                                          tr("Supported formats (*.png *.mp4)")
    );
    if (!fileName.isEmpty())
        lineEditOutputFile->setText(fileName);
}


void DialogWalkThrough::preview(bool b) {
#if 0
    // this single line works: very efficient (in another thread without overhead), but I want a better UI.
    walkThrough()->preview();
    viewer_->update();

#elif 0 // this also works, but low framerate (overhead from std::this_thread::sleep_for() and ProgressLogger)
    if (b) {
        if (!interpolator() || interpolator()->number_of_keyframes() == 0) {
            LOG_IF(WARNING, interpolator()->number_of_keyframes() == 0)
                            << "nothing to preview (camera path is empty). You may import a camera path from a file or"
                               " creat it by adding keyframes";
            previewButton->setChecked(false);
            return;
        }

        static int last_stopped_index = 0;
        const auto &frames = interpolator()->interpolate();

        setEnabled(false);
        LOG(INFO) << "preview started...";

        ProgressLogger progress(frames.size(), true);
        for (int id = last_stopped_index; id < frames.size(); ++id) {
            if (progress.is_canceled()) {
                last_stopped_index = id;
                LOG(INFO) << "preview cancelled";
                break;
            }
            const auto &f = frames[id];
            viewer_->camera()->frame()->setPositionAndOrientation(f.position(), f.orientation());
            const int interval = interpolator()->interpolation_period() / interpolator()->interpolation_speed();
            std::this_thread::sleep_for(std::chrono::milliseconds(interval));
            if (id == frames.size() - 1)  // reaches the end frame
                last_stopped_index = 0;

            progress.next();
        }

        if (last_stopped_index == 0)
            LOG(INFO) << "preview finished";

        setEnabled(true);
        previewButton->setChecked(false);
    }
    else
        previewButton->setChecked(false);

#else

    // this also handles the UI, but a bit complicated (because the Qt GUI actions cannot be triggered in another thread)
    // idea: animation signal -> Qt signals -> Qt action
    auto interpolationStopped = [this]() -> void { emit previewStopped(); };
    static int id_interpolationStopped = 0;
    static StopWatch w;
    if (b) {
        if (!interpolator() || interpolator()->number_of_keyframes() == 0) {
            LOG_IF(WARNING, interpolator()->number_of_keyframes() == 0)
                    << "nothing to preview (camera path is empty). "
                       "You may import a camera path from a file or create it by adding keyframes";
            disconnect(previewButton, SIGNAL(toggled(bool)), this, SLOT(preview(bool)));
            previewButton->setChecked(false);
            connect(previewButton, SIGNAL(toggled(bool)), this, SLOT(preview(bool)));
            return;
        }

        id_interpolationStopped = easy3d::connect(&interpolator()->interpolation_stopped, interpolationStopped);
        QObject::connect(this, &DialogWalkThrough::previewStopped, this, &DialogWalkThrough::onPreviewStopped);

        for (auto w : findChildren<QLabel*>()) w->setEnabled(false);
        for (auto w : findChildren<QPushButton*>()) w->setEnabled(w == previewButton);
        for (auto w : findChildren<QCheckBox*>()) w->setEnabled(false);
        for (auto w : findChildren<QRadioButton*>()) w->setEnabled(false);
        for (auto w : findChildren<QSpinBox*>()) w->setEnabled(false);
        for (auto w : findChildren<QDoubleSpinBox*>()) w->setEnabled(false);
        for (auto w : findChildren<QLineEdit*>()) w->setEnabled(false);
        for (auto w : findChildren<QToolButton*>()) w->setEnabled(false);

        LOG(INFO) << "preview started...";
        w.start();
        interpolator()->start_interpolation();
    }
    else {
        easy3d::disconnect(&interpolator()->interpolation_stopped, id_interpolationStopped);
        QObject::disconnect(this, &DialogWalkThrough::previewStopped, this, &DialogWalkThrough::onPreviewStopped);

        interpolator()->stop_interpolation();
        LOG(INFO) << "preview finished. " << w.time_string() << std::endl;

        for (auto w : findChildren<QLabel*>()) w->setEnabled(true);
        for (auto w : findChildren<QPushButton*>()) w->setEnabled(true);
        for (auto w : findChildren<QCheckBox*>()) w->setEnabled(true);
        for (auto w : findChildren<QRadioButton*>()) w->setEnabled(true);
        for (auto w : findChildren<QSpinBox*>()) w->setEnabled(true);
        for (auto w : findChildren<QDoubleSpinBox*>()) w->setEnabled(true);
        for (auto w : findChildren<QLineEdit*>()) w->setEnabled(true);
        for (auto w : findChildren<QToolButton*>()) w->setEnabled(true);

        setWalkingMode(radioButtonWalkingMode->isChecked());

        viewer_->update();
    }
#endif
}


void DialogWalkThrough::record() {
    if (!interpolator() || interpolator()->number_of_keyframes() == 0) {
        LOG_IF(WARNING, interpolator()->number_of_keyframes() == 0)
                        << "nothing to record (camera path is empty). You may import a camera path from a file or"
                           " creat it by adding keyframes";
        return;
    }

    if (previewButton->isChecked())
        previewButton->setChecked(false);

    // make sure the path is not visible in recording
    const bool visible = walkThrough()->path_visible();
    if (visible)
        walkThrough()->set_path_visible(false);

    const QString file = lineEditOutputFile->text();
    const int fps = spinBoxFPS->value();
    const int bitrate = spinBoxBitRate->value();

    hide();
    QApplication::processEvents();
    LOG(INFO) << "recording started...";
    StopWatch w;
    viewer_->recordAnimation(file, fps, bitrate, true);
    LOG(INFO) << "recording finished. " << w.time_string() << std::endl;

    // restore
    if (visible)
        walkThrough()->set_path_visible(true);
}


void DialogWalkThrough::onPreviewStopped() {
    previewButton->setChecked(false);
}


void DialogWalkThrough::showCameraPath(bool b) {
    walkThrough()->set_path_visible(b);
    if (b) {
        const int count = interpolator()->number_of_keyframes();
        float radius = viewer_->camera()->sceneRadius();
        for (int i=0; i<count; ++i) {
            radius = std::max( radius,
                               distance(viewer_->camera()->sceneCenter(), interpolator()->keyframe(i).position())
            );
        }
        if (radius > viewer_->camera()->sceneRadius())
            viewer_->camera()->setSceneRadius(radius);
    }
    else {
        Box3 box;
        for (auto m : viewer_->models())
            box.add_box(m->bounding_box());
        viewer_->camera()->setSceneBoundingBox(box.min(), box.max());
    }
    viewer_->update();
}


void DialogWalkThrough::exportCameraPathToFile() {
    if (interpolator()->number_of_keyframes() == 0 && interpolator()->number_of_keyframes() == 0) {
        LOG(INFO) << "nothing can be exported (path is empty)";
        return;
    }

    std::string name = "./keyframes.kf";
    if (viewer_->currentModel())
        name = file_system::replace_extension(viewer_->currentModel()->name(), "kf");

    QString suggested_name = QString::fromStdString(name);
    const QString fileName = QFileDialog::getSaveFileName(
            this,
            "Export keyframes to file",
            suggested_name,
            "Keyframe file (*.kf)\n"
            "All formats (*.*)"
    );

    if (fileName.isEmpty())
        return;

    std::ofstream output(fileName.toStdString().c_str());
    if (interpolator()->save_keyframes(output))
        LOG(INFO) << "keyframes saved to file";
}


void DialogWalkThrough::importCameraPathFromFile() {
    std::string dir = "./";
    if (viewer_->currentModel())
        dir = file_system::parent_directory(viewer_->currentModel()->name());
    QString suggested_dir = QString::fromStdString(dir);
    const QString fileName = QFileDialog::getOpenFileName(
            this,
            "Import keyframes from file",
            suggested_dir,
            "Keyframe file (*.kf)\n"
            "All formats (*.*)"
    );

    if (fileName.isEmpty())
        return;

    std::ifstream input(fileName.toStdString().c_str());
    if (interpolator()->read_keyframes(input)) {
        LOG(INFO) << interpolator()->number_of_keyframes() << " keyframes loaded";
        if (walkThrough()->path_visible())
            showCameraPath(true);   // change the scene bbox
        numKeyramesChanged();
    }

    update();
}
