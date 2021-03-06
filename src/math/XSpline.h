// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_MATH_XSPLINE_H_
#define SCANTAILOR_MATH_XSPLINE_H_

#include <QLineF>
#include <QPointF>
#include <vector>

#include "NumericTraits.h"
#include "QuadraticFunction.h"
#include "VirtualFunction.h"
#include "spfit/FittableSpline.h"

/**
 * \brief An open X-Spline.
 *
 * [1] Blanc, C., Schlick, C.: X-splines: a spline model designed for the end-user.
 * http://scholar.google.com/scholar?cluster=2002168279173394147&hl=en&as_sdt=0,5
 */
class XSpline : public spfit::FittableSpline {
 public:
  struct PointAndDerivs {
    QPointF point;

    /**< Point on a spline. */
    QPointF firstDeriv;

    /**< First derivative with respect to t. */
    QPointF secondDeriv; /**< Second derivative with respect to t. */

    /**
     * \brief Curvature at a given point on the spline.
     *
     * The sign indicates curving direction.  Positive signs normally
     * indicate anti-clockwise direction, though in 2D computer graphics
     * it's usually the other way around, as the Y axis points down.
     * In other words, if you rotate your coordinate system so that
     * the X axis aligns with the tangent vector, curvature will be
     * positive if the spline curves towards the positive Y direction.
     */
    double signedCurvature() const;
  };

  int numControlPoints() const override;

  /**
   * Returns the number of segments, that is spans between adjacent control points.
   * Because this class only deals with open splines, the number of segments
   * will always be max(0, numControlPoints() - 1).
   */
  int numSegments() const;

  double controlPointIndexToT(int idx) const;

  /**
   * \brief Appends a control point to the end of the spline.
   *
   * Tension values lie in the range of [-1, 1].
   * \li tension < 0 produces interpolating patches.
   * \li tension == 0 produces sharp angle interpolating patches.
   * \li tension > 0 produces approximating patches.
   */
  void appendControlPoint(const QPointF& pos, double tension);

  /**
   * \brief Inserts a control at a specified position.
   *
   * \p idx is the position where the new control point will end up in.
   * The following control points will be shifted.
   */
  void insertControlPoint(int idx, const QPointF& pos, double tension);

  void eraseControlPoint(int idx);

  QPointF controlPointPosition(int idx) const override;

  void moveControlPoint(int idx, const QPointF& pos) override;

  double controlPointTension(int idx) const;

  void setControlPointTension(int idx, double tension);

  /**
   * \brief Calculates a point on the spline at position t.
   *
   * \param t Position on a spline in the range of [0, 1].
   * \return Point on a spline.
   *
   * \note Calling this function with less then 2 control points
   *       leads to undefined behaviour.
   */
  QPointF pointAt(double t) const;

  /**
   * \brief Calculates a point on the spline plus the first and the second derivatives at position t.
   */
  PointAndDerivs pointAndDtsAt(double t) const;

  /** \see spfit::FittableSpline::linearCombinationAt() */
  void linearCombinationAt(double t, std::vector<LinearCoefficient>& coeffs) const override;

  /**
   * Returns a function equivalent to:
   * \code
   * sum((cp[i].x - cp[i - 1].x)^2 + (cp[i].y - cp[i - 1].y)^2)
   * \endcode
   * Except the returned function is a function of control point displacements,
   * not positions.
   * The sum is calculated across all segments.
   */
  QuadraticFunction controlPointsAttractionForce() const;

  /**
   * Same as the above one, but you provide a range of segments to consider.
   * The range is half-closed: [segBegin, segEnd)
   */
  QuadraticFunction controlPointsAttractionForce(int segBegin, int segEnd) const;

  /**
   * Returns a function equivalent to:
   * \code
   * sum(Vec2d(pointAt(controlPointIndexToT(i)) - pointAt(controlPointIndexToT(i - 1))).squaredNorm())
   * \endcode
   * Except the returned function is a function of control point displacements,
   * not positions.
   * The sum is calculated across all segments.
   */
  QuadraticFunction junctionPointsAttractionForce() const;

  /**
   * Same as the above one, but you provide a range of segments to consider.
   * The range is half-closed: [segBegin, segEnd)
   */
  QuadraticFunction junctionPointsAttractionForce(int segBegin, int segEnd) const;

  /**
   * \brief Finds a point on the spline that's closest to a given point.
   *
   * \param to The point which we trying to minimize the distance to.
   * \param t If provided, the t value corresponding to the found point will be written there.
   * \param accuracy The maximum distance from the found point to the spline.
   * \return The closest point found.
   */
  QPointF pointClosestTo(QPointF to, double* t, double accuracy = 0.2) const;

  QPointF pointClosestTo(QPointF to, double accuracy = 0.2) const;

  /** \see spfit::FittableSpline::sample() */
  void sample(const VirtualFunction<void, const QPointF&, double, SampleFlags>& sink,
              const SamplingParams& params = SamplingParams(),
              double fromT = 0.0,
              double toT = 1.0) const override;

  std::vector<QPointF> toPolyline(const SamplingParams& params = SamplingParams(),
                                  double fromT = 0.0,
                                  double toT = 1.0) const;

  void swap(XSpline& other) { m_controlPoints.swap(other.m_controlPoints); }

 private:
  struct ControlPoint {
    QPointF pos;

    /**
     * Tension is in range of [-1 .. 1] and corresponds to sk as defined in section 5 of [1],
     * not to be confused with sk defined in section 4, which has a range of [0 .. 1].
     */
    double tension;

    ControlPoint() : tension(0) {}

    ControlPoint(const QPointF& p, double tns) : pos(p), tension(tns) {}
  };

  struct TensionDerivedParams;

  class GBlendFunc;
  class HBlendFunc;

  struct DecomposedDerivs;

  QPointF pointAtImpl(int segment, double t) const;

  int linearCombinationFor(LinearCoefficient* coeffs, int segment, double t) const;

  DecomposedDerivs decomposedDerivs(double t) const;

  DecomposedDerivs decomposedDerivsImpl(int segment, double t) const;

  void maybeAddMoreSamples(const VirtualFunction<void, const QPointF&, double, SampleFlags>& sink,
                           double maxSqdistToSpline,
                           double maxSqdistBetweenSamples,
                           double numSegments,
                           double rNumSegments,
                           double prevT,
                           const QPointF& prevPt,
                           double nextT,
                           const QPointF& nextPt) const;

  static double sqDistToLine(const QPointF& pt, const QLineF& line);

  std::vector<ControlPoint> m_controlPoints;
};


inline void swap(XSpline& o1, XSpline& o2) {
  o1.swap(o2);
}

#endif  // ifndef SCANTAILOR_MATH_XSPLINE_H_
