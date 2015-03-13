#ifndef CURVE_CONNECT_AS
#define CURVE_CONNECT_AS

#include <QString>

enum CurveConnectAs {
  CONNECT_AS_FUNCTION_SMOOTH,
  CONNECT_AS_FUNCTION_STRAIGHT,
  CONNECT_AS_RELATION_SMOOTH,
  CONNECT_AS_RELATION_STRAIGHT
};

extern QString curveConnectAsToString (CurveConnectAs curveConnectAs);

#endif // CURVE_CONNECT_AS


