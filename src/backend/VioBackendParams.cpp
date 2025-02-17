/* ----------------------------------------------------------------------------
 * Copyright 2017, Massachusetts Institute of Technology,
 * Cambridge, MA 02139
 * All Rights Reserved
 * Authors: Luca Carlone, et al. (see THANKS for the full author list)
 * See LICENSE for the license information
 * -------------------------------------------------------------------------- */

/**
 * @file   VioBackendParams.cpp
 * @brief  Class parsing the parameters for the VIO's Backend from a YAML file.
 * @author Antoni Rosinol, Luca Carlone
 */

#include "kimera-vio/backend/VioBackendParams.h"

#include <utility>

namespace VIO {

BackendParams::BackendParams() : PipelineParams("Backend Parameters") {
  // Trivial sanity checks.
  CHECK_GE(horizon_, 0);
  CHECK_GE(numOptimize_, 0);
}

void BackendParams::setIsam2Params(const BackendParams& vio_params,
                                   gtsam::ISAM2Params* isam_param) {
  CHECK_NOTNULL(isam_param);
  // iSAM2 SETTINGS
  if (vio_params.useDogLeg_) {
    gtsam::ISAM2DoglegParams dogleg_params;
    dogleg_params.wildfireThreshold = vio_params.wildfire_threshold_;
    // dogleg_params.adaptationMode;
    // dogleg_params.initialDelta;
    // dogleg_params.setVerbose(false); // only for debugging.
    isam_param->optimizationParams = dogleg_params;
  } else {
    gtsam::ISAM2GaussNewtonParams gauss_newton_params;
    gauss_newton_params.wildfireThreshold = vio_params.wildfire_threshold_;
    isam_param->optimizationParams = gauss_newton_params;
  }

  // TODO (Toni): remove hardcoded
  // Cache Linearized Factors seems to improve performance.
  isam_param->cacheLinearizedFactors=true;
  isam_param->relinearizeThreshold = vio_params.relinearizeThreshold_;
  isam_param->relinearizeSkip = vio_params.relinearizeSkip_;
  isam_param->findUnusedFactorSlots = true;
  // isam_param->enablePartialRelinearizationCheck = true;
  isam_param->evaluateNonlinearError=false;  // only for debugging
  isam_param->enableDetailedResults = false;     // only for debugging.
  isam_param->factorization = gtsam::ISAM2Params::CHOLESKY;  // QR
}

bool BackendParams::equals(const BackendParams& vp2, double tol) const {
  return equalsVioBackendParams(vp2, tol);
}

void BackendParams::print() const { printVioBackendParams(); }

bool BackendParams::parseYAML(const std::string& filepath) {
  YamlParser yaml_parser(filepath);
  return parseYAMLVioBackendParams(yaml_parser);
}

bool BackendParams::parseYAMLVioBackendParams(const YamlParser& yaml_parser) {
  // INITIALIZATION
  yaml_parser.getYamlParam("autoInitialize", &autoInitialize_);
  yaml_parser.getYamlParam("roundOnAutoInitialize", &roundOnAutoInitialize_);
  yaml_parser.getYamlParam("initialPositionSigma", &initialPositionSigma_);
  yaml_parser.getYamlParam("initialRollPitchSigma", &initialRollPitchSigma_);
  yaml_parser.getYamlParam("initialYawSigma", &initialYawSigma_);
  yaml_parser.getYamlParam("initialVelocitySigma", &initialVelocitySigma_);
  yaml_parser.getYamlParam("initialAccBiasSigma", &initialAccBiasSigma_);
  yaml_parser.getYamlParam("initialGyroBiasSigma", &initialGyroBiasSigma_);

  // VISION PARAMS
  int linearization_mode_id;
  yaml_parser.getYamlParam("linearizationMode", &linearization_mode_id);
  switch (linearization_mode_id) {
    case 0:
      linearizationMode_ = gtsam::HESSIAN;
      break;
    case 1:
      linearizationMode_ = gtsam::IMPLICIT_SCHUR;
      break;
    case 2:
      linearizationMode_ = gtsam::JACOBIAN_Q;
      break;
    case 3:
      linearizationMode_ = gtsam::JACOBIAN_SVD;
      break;
    default:
      LOG(FATAL) << "Wrong linearizationMode in VIO Backend parameters.";
  }

  int degeneracy_mode_id;
  yaml_parser.getYamlParam("degeneracyMode", &degeneracy_mode_id);
  switch (degeneracy_mode_id) {
    case 0:
      degeneracyMode_ = gtsam::IGNORE_DEGENERACY;
      break;
    case 1:
      degeneracyMode_ = gtsam::ZERO_ON_DEGENERACY;
      break;
    case 2:
      degeneracyMode_ = gtsam::HANDLE_INFINITY;
      break;
    default:
      LOG(FATAL) << "Wrong degeneracyMode in VIO Backend parameters.";
  }

  yaml_parser.getYamlParam("smartNoiseSigma", &smartNoiseSigma_);
  yaml_parser.getYamlParam("rankTolerance", &rankTolerance_);
  yaml_parser.getYamlParam("landmarkDistanceThreshold",
                           &landmarkDistanceThreshold_);
  yaml_parser.getYamlParam("outlierRejection", &outlierRejection_);
  yaml_parser.getYamlParam("retriangulationThreshold",
                           &retriangulationThreshold_);
  yaml_parser.getYamlParam("addBetweenStereoFactors",
                           &addBetweenStereoFactors_);
  yaml_parser.getYamlParam("betweenRotationPrecision",
                           &betweenRotationPrecision_);
  yaml_parser.getYamlParam("betweenTranslationPrecision",
                           &betweenTranslationPrecision_);

  // OPTIMIZATION PARAMS
  yaml_parser.getYamlParam("relinearizeThreshold", &relinearizeThreshold_);
  yaml_parser.getYamlParam("relinearizeSkip", &relinearizeSkip_);
  yaml_parser.getYamlParam("zeroVelocitySigma", &zeroVelocitySigma_);
  yaml_parser.getYamlParam("noMotionPositionSigma", &noMotionPositionSigma_);
  yaml_parser.getYamlParam("noMotionRotationSigma", &noMotionRotationSigma_);
  yaml_parser.getYamlParam("constantVelSigma", &constantVelSigma_);
  yaml_parser.getYamlParam("numOptimize", &numOptimize_);
  yaml_parser.getYamlParam("horizon", &horizon_);
  yaml_parser.getYamlParam("wildfire_threshold", &wildfire_threshold_);
  yaml_parser.getYamlParam("useDogLeg", &useDogLeg_);

  return true;
}

bool BackendParams::equalsVioBackendParams(const BackendParams& vp2,
                                           double tol) const {
  return
      // INITIALIZATION
      (autoInitialize_ == vp2.autoInitialize_) &&
      initial_ground_truth_state_.equals(vp2.initial_ground_truth_state_) &&
      (roundOnAutoInitialize_ == vp2.roundOnAutoInitialize_) &&
      (fabs(initialPositionSigma_ - vp2.initialPositionSigma_) <= tol) &&
      (fabs(initialRollPitchSigma_ - vp2.initialRollPitchSigma_) <= tol) &&
      (fabs(initialYawSigma_ - vp2.initialYawSigma_) <= tol) &&
      (fabs(initialVelocitySigma_ - vp2.initialVelocitySigma_) <= tol) &&
      (fabs(initialAccBiasSigma_ - vp2.initialAccBiasSigma_) <= tol) &&
      (fabs(initialGyroBiasSigma_ - vp2.initialGyroBiasSigma_) <= tol) &&
      // VISION PARAMS
      (linearizationMode_ == vp2.linearizationMode_) &&
      (degeneracyMode_ == vp2.degeneracyMode_) &&
      (fabs(smartNoiseSigma_ - vp2.smartNoiseSigma_) <= tol) &&
      (fabs(rankTolerance_ - vp2.rankTolerance_) <= tol) &&
      (fabs(landmarkDistanceThreshold_ - vp2.landmarkDistanceThreshold_) <=
       tol) &&
      (fabs(outlierRejection_ - vp2.outlierRejection_) <= tol) &&
      (fabs(retriangulationThreshold_ - vp2.retriangulationThreshold_) <=
       tol) &&
      (addBetweenStereoFactors_ == vp2.addBetweenStereoFactors_) &&
      (fabs(betweenRotationPrecision_ - vp2.betweenRotationPrecision_) <=
       tol) &&
      (fabs(betweenTranslationPrecision_ - vp2.betweenTranslationPrecision_) <=
       tol) &&
      // OPTIMIZATION PARAMS
      (fabs(relinearizeThreshold_ - vp2.relinearizeThreshold_) <= tol) &&
      (relinearizeSkip_ == vp2.relinearizeSkip_) &&
      (fabs(zeroVelocitySigma_ - vp2.zeroVelocitySigma_) <= tol) &&
      (fabs(noMotionPositionSigma_ - vp2.noMotionPositionSigma_) <= tol) &&
      (fabs(noMotionRotationSigma_ - vp2.noMotionRotationSigma_) <= tol) &&
      (fabs(constantVelSigma_ - vp2.constantVelSigma_) <= tol) &&
      (numOptimize_ == vp2.numOptimize_) && (horizon_ == vp2.horizon_) &&
      (wildfire_threshold_ == vp2.wildfire_threshold_) &&
      (useDogLeg_ == vp2.useDogLeg_);
}

void BackendParams::printVioBackendParams() const {
  static const int kCenter = PipelineParams::kTotalWidth / 2;
  std::stringstream out;
  PipelineParams::print(
      out,
      std::string(kCenter, '.') + "** Initialization parameters **",
      "",
      "Automatic Initialization",
      autoInitialize_,
      "Round on autoinit",
      roundOnAutoInitialize_,
      "Sigmas:",
      "",
      "Initial Position Sigma",
      initialPositionSigma_,
      "Initiail Roll Pitch Sigma",
      initialRollPitchSigma_,
      "Initial Yaw Sigma",
      initialYawSigma_,
      "Initial Velocity Sigma",
      initialVelocitySigma_,
      "Initial Acc Bias Sigma",
      initialAccBiasSigma_,
      "Initial Gyro Bias Sigma",
      initialGyroBiasSigma_,
      std::string(kCenter, '.') + "** Vision parameters **",
      "",
      "Linearization Mode: hessian, implicit_schur, jacobian_q, jacobian_svd",
      linearizationMode_,
      "Degeneracy Mode: ignore_degeneracy, zero_on_degeneracy, handle_infinity",
      degeneracyMode_,
      "Rank Tolerance",
      rankTolerance_,
      "Landmark Distance Threshold",
      landmarkDistanceThreshold_,
      "Outlier Rejection",
      outlierRejection_,
      "Retriangulation Threshold",
      retriangulationThreshold_,
      "Add Btw Stereo Factors",
      addBetweenStereoFactors_,
      "Btw Rotation Precision",
      betweenRotationPrecision_,
      "Btw Translation Precision",
      betweenTranslationPrecision_,
      std::string(kCenter, '.') + "** Optimization parameters **",
      "",
      "Relinearize Threshold",
      relinearizeThreshold_,
      "Relinearize Skip",
      relinearizeSkip_,
      "Zero Velocity Sigma",
      zeroVelocitySigma_,
      "No Motion Position Sigma",
      noMotionPositionSigma_,
      "No Motion Rotation Sigma",
      noMotionRotationSigma_,
      "Constant Velocity Sigma",
      constantVelSigma_,
      "Optimization Iterations",
      numOptimize_,
      "Horizon",
      horizon_,
      "Isam Wildfire Threshold",
      wildfire_threshold_,
      "Use Dog Leg",
      useDogLeg_);
  LOG(INFO) << out.str();
  LOG(INFO) << "** Backend Iinitialization Parameters **\n"
            << "initial_ground_truth_state_: ";
  initial_ground_truth_state_.print();
}

}  // namespace VIO
