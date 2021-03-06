#include <grad_traj_optimization/edt_environment.h>

namespace dyn_planner
{
/* ============================== edt_environment ============================== */
void EDTEnvironment::init()
{
}

void EDTEnvironment::setMap(shared_ptr<SDFMap> map)
{
  this->sdf_map_ = map;
  resolution_inv_ = 1 / sdf_map_->getResolution();
}

void EDTEnvironment::setObjPrediction(ObjPrediction prediction)
{
  this->obj_prediction_ = prediction;
}

void EDTEnvironment::setObjScale(ObjScale scale)
{
  this->obj_scale_ = scale;
}

double EDTEnvironment::distToBox(int idx, const Eigen::Vector3d& pos, const double& time)
{
  // Eigen::Vector3d pos_box = obj_prediction_->at(idx).evaluate(time);
  Eigen::Vector3d pos_box = obj_prediction_->at(idx).evaluateConstVel(time);

  Eigen::Vector3d box_max = pos_box + 0.5 * obj_scale_->at(idx);
  Eigen::Vector3d box_min = pos_box - 0.5 * obj_scale_->at(idx);

  Eigen::Vector3d dist;

  for (int i = 0; i < 3; i++)
  {
    dist(i) =
        pos(i) >= box_min(i) && pos(i) <= box_max(i) ? 0.0 : min(fabs(pos(i) - box_min(i)), fabs(pos(i) - box_max(i)));
  }

  return dist.norm();

  /* point on the surface */
  // Eigen::Vector3d dir = pos - pos_box;
  // double approx_dist = dir.norm() - radius.norm();
  // if (approx_dist > sdf_map_->getIgnoreRadius())
  //   return approx_dist;

  // /* find the closest point on box */
  // double ratio = std::numeric_limits<double>::max();
  // for (int i = 0; i < 3; ++i)
  // {
  //   double rt = dir(i) >= 0.0 ? radius(i) / dir(i) : -radius(i) / dir(i);
  //   if (rt < 1.0 && rt < ratio)
  //     ratio = rt;
  // }
  // Eigen::Vector3d closest = pos_box + ratio * dir;
  // double dist = (pos - closest).norm();
}

double EDTEnvironment::minDistToAllBox(const Eigen::Vector3d& pos, const double& time)
{
  double dist = 10000000.0;
  for (int i = 0; i < obj_prediction_->size(); i++)
  {
    double di = distToBox(i, pos, time);
    if (di < dist)
      dist = di;
  }

  return dist;
}

void EDTEnvironment::evaluateEDTWithGrad(const Eigen::Vector3d& pos, const double& time, double& dist,
                                         Eigen::Vector3d& grad)
{
  vector<Eigen::Vector3d> pos_vec;
  Eigen::Vector3d diff;
  sdf_map_->getInterpolationData(pos, pos_vec, diff);

  /* ---------- value from surrounding position ---------- */
  double values[2][2][2];
  Eigen::Vector3d pt;
  for (int x = 0; x < 2; x++)
    for (int y = 0; y < 2; y++)
      for (int z = 0; z < 2; z++)
      {
        pt = pos_vec[4 * x + 2 * y + z];
        double d1 = sdf_map_->getDistance(pt);

        if (time < 0.0)
        {
          values[x][y][z] = d1;
        }
        else
        {
          double d2 = minDistToAllBox(pt, time);
          values[x][y][z] = min(d1, d2);
        }
      }

  /* ---------- use trilinear interpolation ---------- */
  double v00 = (1 - diff(0)) * values[0][0][0] + diff(0) * values[1][0][0];
  double v01 = (1 - diff(0)) * values[0][0][1] + diff(0) * values[1][0][1];
  double v10 = (1 - diff(0)) * values[0][1][0] + diff(0) * values[1][1][0];
  double v11 = (1 - diff(0)) * values[0][1][1] + diff(0) * values[1][1][1];

  double v0 = (1 - diff(1)) * v00 + diff(1) * v10;
  double v1 = (1 - diff(1)) * v01 + diff(1) * v11;

  dist = (1 - diff(2)) * v0 + diff(2) * v1;

  grad[2] = (v1 - v0) * resolution_inv_;
  grad[1] = ((1 - diff[2]) * (v10 - v00) + diff[2] * (v11 - v01)) * resolution_inv_;
  grad[0] = (1 - diff[2]) * (1 - diff[1]) * (values[1][0][0] - values[0][0][0]);
  grad[0] += (1 - diff[2]) * diff[1] * (values[1][1][0] - values[0][1][0]);
  grad[0] += diff[2] * (1 - diff[1]) * (values[1][0][1] - values[0][0][1]);
  grad[0] += diff[2] * diff[1] * (values[1][1][1] - values[0][1][1]);

  grad[0] *= resolution_inv_;
}

double EDTEnvironment::evaluateCoarseEDT(const Eigen::Vector3d& pos, const double& time)
{
  double d1 = sdf_map_->getDistance(pos);
  if (time < 0.0)
  {
    return d1;
  }
  else
  {
    double d2 = minDistToAllBox(pos, time);
    return min(d1, d2);
  }
}

// EDTEnvironment::
}  // namespace dyn_planner