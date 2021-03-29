#ifndef COMMON_MTH_HPP
#define COMMON_MTH_HPP

#include <math.h>
#include <type_traits>
#include <initializer_list>

namespace mth {

double const pi = 3.1415926535897932384626433832795028841971693993751058209749445923078164062;
    
template<typename T>
inline T sqrt(T const& v);

template<typename T>
inline T pow(T const& b, T const& e);
    
template<typename T>
inline T cos(T const& v);
    
template<typename T>
inline T sin(T const& v);
    
template<typename T>
inline T tan(T const& v);
    
template<typename T>
inline T acos(T const& v);
  
template<typename T>
inline T asin(T const& v);
  
template<typename T>
inline T atan(T const& v);
  
template<typename T>
inline T atan2(T const& a, T const& b);
    
template<typename T>
inline T abs(T const& v);
    
template<>
inline float sqrt<float> (float const& v) {
  return ::sqrtf(v);
}
template<>
inline double sqrt<double> (double const& v) {
  return ::sqrt(v);
}
template<>
inline long double sqrt<long double> (long double const& v) {
  return ::sqrtl(v);
}

template<>
inline float pow<float> (float const& b, float const& e) {
  return ::powf(b, e);
}
template<>
inline double pow<double> (double const& b, double const& e) {
  return ::pow(b, e);
}
template<>
inline long double pow<long double> (long double const& b, long double const& e) {
  return ::powl(b, e);
}
    
template<>
inline float cos<float> (float const& v) {
  return ::cosf(v);
}
template<>
inline double cos<double> (double const& v) {
  return ::cos(v);
}
template<>
inline long double cos<long double> (long double const& v) {
  return ::cosl(v);
}
    
template<>
inline float sin<float> (float const& v) {
  return ::sinf(v);
}
template<>
inline double sin<double> (double const& v) {
  return ::sin(v);
}
template<>
inline long double sin<long double> (long double const& v) {
  return ::sinl(v);
}
    
template<>
inline float tan<float> (float const& v) {
  return ::tanf(v);
}
template<>
inline double tan<double> (double const& v) {
  return ::tan(v);
}
template<>
inline long double tan<long double> (long double const& v) {
  return ::tanl(v);
}
    
template<>
inline float acos<float> (float const& v) {
  return ::acosf(v);
}
template<>
inline double acos<double> (double const& v) {
  return ::acos(v);
}
template<>
inline long double acos<long double> (long double const& v) {
  return ::acosl(v);
}
    
template<>
inline float asin<float> (float const& v) {
  return ::asinf(v);
}
template<>
inline double asin<double> (double const& v) {
  return ::asin(v);
}
template<>
inline long double asin<long double> (long double const& v) {
  return ::asinl(v);
}
    
template<>
inline float atan<float> (float const& v) {
  return ::atanf(v);
}
template<>
inline double atan<double> (double const& v) {
  return ::atan(v);
}
template<>
inline long double atan<long double> (long double const& v) {
  return ::atanl(v);
}
    
template<>
inline float atan2<float> (float const& a, float const& b) {
  return ::atan2f(a, b);
}
template<>
inline double atan2<double> (double const& a, double const& b) {
  return ::atan2(a, b);
}
template<>
inline long double atan2<long double> (long double const& a, long double const& b) {
  return ::atan2l(a, b);
}
    
template<>
inline float abs<float>(float const& v) {
  return ::fabsf(v);
}
template<>
inline double abs<double>(double const& v) {
  return ::fabs(v);
}
template<>
inline long double abs<long double>(long double const& v) {
  return ::fabsl(v);
}
    
template<typename T>
inline constexpr int signum(T x, std::false_type) {
  return static_cast<T>(0) <= x;
}
    
template<typename T>
inline constexpr int signum(T x, std::true_type) {
  return (static_cast<T>(0) <= x) - (x < static_cast<T>(0));
}
    
template<typename T>
inline constexpr int signum(T x) {
  return signum(x, std::is_signed<T>());
}

template<typename T, unsigned int R, unsigned int C>
class matrix {
  T _cells[R * C];
  
public:
  matrix() {}
  matrix(std::initializer_list<T> l) {
    //static_assert(l.size() == R * C, "Size of initializer list does not match matrix dimensions");
    auto it = l.begin();
    for (unsigned int i = 0; i < R * C; ++i) {
      _cells[i] = *it; ++it;
    }
  }
  
  inline T const& operator () (unsigned int row, unsigned int col = 0) const {
    return _cells[C * row + col];
  }
  inline T& operator () (unsigned int row, unsigned int col = 0) {
    return _cells[C * row + col];
  }
  inline matrix<T, R, C> const& operator += (matrix<T, R, C> const& m) {
    for (unsigned int i = 0; i < R * C; ++i) {
      _cells[i] += m._cells[i];
    }
    return *this;
  }
  inline matrix<T, R, C> const& operator -= (matrix<T, R, C> const& m) {
    for (unsigned int i = 0; i < R * C; ++i) {
      _cells[i] -= m._cells[i];
    }
    return *this;
  }
  inline matrix<T, R, C> const& operator += (T const& s) {
    for (unsigned int i = 0; i < R * C; ++i) {
      _cells[i] += s;
    }
    return *this;
  }
  inline matrix<T, R, C> const& operator -= (T const& s) {
    for (unsigned int i = 0; i < R * C; ++i) {
      _cells[i] -= s;
    }
    return *this;
  }
  template<typename U>
  inline matrix<T, R, C> const& operator *= (U const& s) {
    for (unsigned int i = 0; i < R * C; ++i) {
      _cells[i] *= s;
    }
    return *this;
  }
  template<typename U>
  inline matrix<T, R, C> const& operator /= (U const& s) {
    for (unsigned int i = 0; i < R * C; ++i) {
      _cells[i] /= s;
    }
    return *this;
  }
}; // matrix

////////////////////////////////////////////////////////////////////////////////

template<typename T, unsigned int R, unsigned int C>
inline auto identity() {
  matrix<T, R, C> m;
  for (unsigned int row = 0; row < R; ++row) {
    for (unsigned int col = 0; col < C; ++col) {
      m(row,col) = row == col ? T(1) : T(0);
    }
  }
  return m;
}

template<typename T, unsigned int R, unsigned int C>
inline auto diagonal(T const& d) {
  matrix<T, R, C> m;
  for (unsigned int row = 0; row < R; ++row) {
    for (unsigned int col = 0; col < C; ++col) {
      m(row,col) = d;
    }
  }
  return m;
}

////////////////////////////////////////////////////////////////////////////////

template<typename T, unsigned int R, unsigned int C>
inline matrix<T, 1, C>
row(matrix<T, R, C> const& m, unsigned int r) {
  matrix<T, 1, C> result;
  for (int col = 0; col < C; ++col) {
    result(0, col) = m(r, col);
  }
  return result;
}
template<typename T, unsigned int R, unsigned int C>
inline matrix<T, R, 1>
col(matrix<T, R, C> const& m, unsigned int c) {
  matrix<T, R, 1> result;
  for (int row = 0; row < R; ++row) {
    result(row, 0) = m(row, c);
  }
  return result;
}
template<unsigned int NR, unsigned int NC, typename T, unsigned int R, unsigned int C>
inline matrix<T, NR, NC>
sub(matrix<T, R, C> const& m, unsigned int row_offset, unsigned int col_offset) {
  matrix<T, NR, NC> result;
  for (unsigned int row = 0; row < NR; ++row) {
    for (unsigned int col = 0; col < NC; ++col) {
      result(row, col) = m(row_offset + row, col_offset + col);
    }
  }
  return result;
}

////////////////////////////////////////////////////////////////////////////////

template<typename T, unsigned int R, unsigned int C>
inline matrix<T, R, C>
operator + (matrix<T, R, C> const& m0, matrix<T, R, C> const& m1) {
  matrix<T, R, C> result(m0);
  result += m1;
  return result;
}
template<typename T, unsigned int R, unsigned int C>
inline matrix<T, R, C>
operator - (matrix<T, R, C> const& m0, matrix<T, R, C> const& m1) {
  matrix<T, R, C> result(m0);
  result -= m1;
  return result;
}
template<typename T, unsigned int R, unsigned int C>
inline matrix<T, R, C>
operator + (matrix<T, R, C> const& m, T const& s) {
  matrix<T, R, C> result(m);
  result += s;
  return result;
}
template<typename T, unsigned int R, unsigned int C>
inline matrix<T, R, C>
operator - (matrix<T, R, C> const& m, T const& s) {
  matrix<T, R, C> result(m);
  result -= s;
  return result;
}
template<typename T, unsigned int R, unsigned int C>
inline matrix<T, R, C>
operator - (matrix<T, R, C> const& m) {
  matrix<T, R, C> result;
  for (unsigned int row = 0; row < R; ++row) {
    for (unsigned int col = 0; col < C; ++col) {
      result(row, col) = -m(row, col);
    }
  }
  return result;
}

////////////////////////////////////////////////////////////////////////////////

template<typename T, unsigned int R, unsigned int N, unsigned int C>
inline matrix<T, R, N>
operator * (matrix<T, R, N> const& m0, matrix<T, N, C> const& m1) {
  matrix<T, R, C> result;
  for (unsigned int row = 0; row < R; ++row) {
    for (unsigned int col = 0; col < C; ++col) {
      T cell(0);
      for (unsigned int i = 0; i < N; ++i) {
        cell += (m0(row, i) * m1(i, col));
      }
      result(row, col) = cell;
    }
  }
  return result;
}
template<typename T, unsigned int R, unsigned int C>
inline matrix<T, R, C>
operator * (matrix<T, R, C> const& m, T const& s) {
  matrix<T, R, C> result;
  for (unsigned int row = 0; row < R; ++row) {
    for (unsigned int col = 0; col < C; ++col) {
      result(row, col) = m(row, col) * s;
    }
  }
  return result;
}
template<typename T, unsigned int R, unsigned int C>
inline matrix<T, R, C>
operator * (T const& s, matrix<T, R, C> const& m) {
  matrix<T, R, C> result;
  for (unsigned int row = 0; row < R; ++row) {
    for (unsigned int col = 0; col < C; ++col) {
      result(row, col) = s * m(row, col);
    }
  }
  return result;
}
template<typename T, unsigned int R, unsigned int C>
inline matrix<T, R, C>
operator / (matrix<T, R, C> const& m, T const& s) {
  matrix<T, R, C> result;
  for (unsigned int row = 0; row < R; ++row) {
    for (unsigned int col = 0; col < C; ++col) {
      result(row, col) = m(row, col) / s;
    }
  }
  return result;
}
    
template<typename T, unsigned int R, unsigned int C>
inline matrix<T, C, R>
transpose(matrix<T, R, C> const& m) {
  matrix<T, C, R> result;
  for (unsigned int row = 0; row < R; ++row) {
    for (unsigned int col = 0; col < C; ++col) {
      result(col, row) = m(row, col);
    }
  }
  return result;
}
    
template<typename T>
inline auto det(matrix<T, 2, 2> const& m) -> decltype(T() * T()) {
  return m(0,0) * m(1,1) - m(1,0) * m(0,1);
}
template<typename T>
inline auto det(matrix<T, 3, 3> const& m) -> decltype(T() * T() * T()) {
  return   m(0,0) * (m(1,1) * m(2,2) - m(1,2) * m(2,1))
    + m(0,1) * (m(1,2) * m(2,0) - m(1,0) * m(2,2))
    + m(0,2) * (m(1,0) * m(2,1) - m(1,1) * m(2,0));
}
template<typename T>
inline auto det(matrix<T, 4, 4> const& m) {
  auto t0 = m(2,2) * m(3,3) - m(3,2) * m(2,3);
  auto t1 = m(2,1) * m(3,3) - m(3,1) * m(2,3);
  auto t2 = m(2,0) * m(3,3) - m(3,0) * m(2,3);
  auto t3 = m(2,1) * m(3,2) - m(3,1) * m(2,2);
  auto t4 = m(2,0) * m(3,2) - m(3,0) * m(2,2);
  auto t5 = m(2,0) * m(3,1) - m(3,0) * m(2,1);
  return m(0,0) * (m(1,1) * t0 - m(1,2) * t1 + m(1,3) * t3)
    - m(0,1) * (m(1,0) * t0 - m(1,2) * t2 + m(1,3) * t4)
    + m(0,2) * (m(1,0) * t1 - m(1,1) * t2 + m(1,3) * t5)
    - m(0,3) * (m(1,0) * t3 - m(1,1) * t4 + m(1,2) * t5);
}
    
template<typename T>
inline matrix<T, 2, 2> inverse(matrix<T, 2, 2> const& m) {
  return matrix<T,2,2>{
    m(1,1), -m(0,1),
    -m(1,0), m(0,0)
  } / det(m);
}
template<typename T>
inline auto inverse(matrix<T, 3, 3> const& m) {
  return matrix<T,3,3>{
    m(1,1) * m(2,2) - m(1,2) * m(2,1), m(0,2) * m(2,1) - m(0,1) * m(2,2), m(0,1) * m(1,2) - m(0,2) * m(1,1),
    m(1,2) * m(2,0) - m(1,0) * m(2,2), m(0,0) * m(2,2) - m(0,2) * m(2,0), m(0,2) * m(1,0) - m(0,0) * m(1,2),
    m(1,0) * m(2,1) - m(1,1) * m(2,0), m(0,1) * m(2,0) - m(0,0) * m(2,1), m(0,0) * m(1,1) - m(0,1) * m(1,0)
  } / det(m);
}
template<typename T>
inline auto inverse(matrix<T, 4, 4> const& m) {
  T m00 =   m(1, 1) * m(2, 2) * m(3, 3)
    - m(1, 1) * m(2, 3) * m(3, 2)
    - m(2, 1) * m(1, 2) * m(3, 3)
    + m(2, 1) * m(1, 3) * m(3, 2)
    + m(3, 1) * m(1, 2) * m(2, 3)
    - m(3, 1) * m(1, 3) * m(2, 2);
  T m10 = - m(1, 0) * m(2, 2) * m(3, 3)
    + m(1, 0) * m(2, 3) * m(3, 2)
    + m(2, 0) * m(1, 2) * m(3, 3)
    - m(2, 0) * m(1, 3) * m(3, 2)
    - m(3, 0) * m(1, 2) * m(2, 3)
    + m(3, 0) * m(1, 3) * m(2, 2);
  T m20 =   m(1, 0) * m(2, 1) * m(3, 3)
    - m(1, 0) * m(2, 3) * m(3, 1)
    - m(2, 0) * m(1, 1) * m(3, 3)
    + m(2, 0) * m(1, 3) * m(3, 1)
    + m(3, 0) * m(1, 1) * m(2, 3)
    - m(3, 0) * m(1, 3) * m(2, 1);
  T m30 = - m(1, 0) * m(2, 1) * m(3, 2)
    + m(1, 0) * m(2, 2) * m(3, 1)
    + m(2, 0) * m(1, 1) * m(3, 2)
    - m(2, 0) * m(1, 2) * m(3, 1)
    - m(3, 0) * m(1, 1) * m(2, 2)
    + m(3, 0) * m(1, 2) * m(2, 1);
        
  T m01 = - m(0, 1) * m(2, 2) * m(3, 3)
    + m(0, 1) * m(2, 3) * m(3, 2)
    + m(2, 1) * m(0, 2) * m(3, 3)
    - m(2, 1) * m(0, 3) * m(3, 2)
    - m(3, 1) * m(0, 2) * m(2, 3)
    + m(3, 1) * m(0, 3) * m(2, 2);
  T m11 =   m(0, 0) * m(2, 2) * m(3, 3)
    - m(0, 0) * m(2, 3) * m(3, 2)
    - m(2, 0) * m(0, 2) * m(3, 3)
    + m(2, 0) * m(0, 3) * m(3, 2)
    + m(3, 0) * m(0, 2) * m(2, 3)
    - m(3, 0) * m(0, 3) * m(2, 2);
  T m21 = - m(0, 0) * m(2, 1) * m(3, 3)
    + m(0, 0) * m(2, 3) * m(3, 1)
    + m(2, 0) * m(0, 1) * m(3, 3)
    - m(2, 0) * m(0, 3) * m(3, 1)
    - m(3, 0) * m(0, 1) * m(2, 3)
    + m(3, 0) * m(0, 3) * m(2, 1);
  T m31 =   m(0, 0) * m(2, 1) * m(3, 2)
    - m(0, 0) * m(2, 2) * m(3, 1)
    - m(2, 0) * m(0, 1) * m(3, 2)
    + m(2, 0) * m(0, 2) * m(3, 1)
    + m(3, 0) * m(0, 1) * m(2, 2)
    - m(3, 0) * m(0, 2) * m(2, 1);

  T m02 =   m(0, 1) * m(1, 2) * m(3, 3)
    - m(0, 1) * m(1, 3) * m(3, 2)
    - m(1, 1) * m(0, 2) * m(3, 3)
    + m(1, 1) * m(0, 3) * m(3, 2)
    + m(3, 1) * m(0, 2) * m(1, 3)
    - m(3, 1) * m(0, 3) * m(1, 2);
  T m12 = - m(0, 0) * m(1, 2) * m(3, 3)
    + m(0, 0) * m(1, 3) * m(3, 2)
    + m(1, 0) * m(0, 2) * m(3, 3)
    - m(1, 0) * m(0, 3) * m(3, 2)
    - m(3, 0) * m(0, 2) * m(1, 3)
    + m(3, 0) * m(0, 3) * m(1, 2);
  T m22 =   m(0, 0) * m(1, 1) * m(3, 3)
    - m(0, 0) * m(1, 3) * m(3, 1)
    - m(1, 0) * m(0, 1) * m(3, 3)
    + m(1, 0) * m(0, 3) * m(3, 1)
    + m(3, 0) * m(0, 1) * m(1, 3)
    - m(3, 0) * m(0, 3) * m(1, 1);
  T m32 = - m(0, 0) * m(1, 1) * m(3, 2)
    + m(0, 0) * m(1, 2) * m(3, 1)
    + m(1, 0) * m(0, 1) * m(3, 2)
    - m(1, 0) * m(0, 2) * m(3, 1)
    - m(3, 0) * m(0, 1) * m(1, 2)
    + m(3, 0) * m(0, 2) * m(1, 1);

  T m03 = - m(0, 1) * m(1, 2) * m(2, 3)
    + m(0, 1) * m(1, 3) * m(2, 2)
    + m(1, 1) * m(0, 2) * m(2, 3)
    - m(1, 1) * m(0, 3) * m(2, 2)
    - m(2, 1) * m(0, 2) * m(1, 3)
    + m(2, 1) * m(0, 3) * m(1, 2);
  T m13 =   m(0, 0) * m(1, 2) * m(2, 3)
    - m(0, 0) * m(1, 3) * m(2, 2)
    - m(1, 0) * m(0, 2) * m(2, 3)
    + m(1, 0) * m(0, 3) * m(2, 2)
    + m(2, 0) * m(0, 2) * m(1, 3)
    - m(2, 0) * m(0, 3) * m(1, 2);
  T m23 = - m(0, 0) * m(1, 1) * m(2, 3)
    + m(0, 0) * m(1, 3) * m(2, 1)
    + m(1, 0) * m(0, 1) * m(2, 3)
    - m(1, 0) * m(0, 3) * m(2, 1)
    - m(2, 0) * m(0, 1) * m(1, 3)
    + m(2, 0) * m(0, 3) * m(1, 1);
  T m33 =   m(0, 0) * m(1, 1) * m(2, 2)
    - m(0, 0) * m(1, 2) * m(2, 1)
    - m(1, 0) * m(0, 1) * m(2, 2)
    + m(1, 0) * m(0, 2) * m(2, 1)
    + m(2, 0) * m(0, 1) * m(1, 2)
    - m(2, 0) * m(0, 2) * m(1, 1);

  T det = m(0, 0) * m00 + m(0, 1) * m10 + m(0, 2) * m20 + m(0, 3) * m30;
  return matrix<T, 4, 4>{
    m00, m01, m02, m03,
    m10, m11, m12, m13,
    m20, m21, m22, m23,
    m30, m31, m32, m33
  } / det;
}

////////////////////////////////////////////////////////////////////////////////

// special case: column vector
template<typename T, unsigned int R>
using vector = matrix<T, R, 1>;

template <typename T0, typename T1, unsigned int N>
inline auto dot(vector<T0,N> const& v0, vector<T1,N> const& v1) {
  auto result = v0(0) * v1(0);
  for (unsigned int i = 1; i < N; ++i) {
    result += v0(i) * v1(i);
  }
  return result;
}
    
template <typename T, unsigned int N>
inline auto hadamard(vector<T,N> const& v0, vector<T,N> const& v1) {
  vector<T,N> result;
  for (unsigned int i = 0; i < N; ++i) {
    result(i) = v0(i) * v1(i);
  }
  return result;
}
    
template <typename T, unsigned int N>
inline T length2(vector<T,N> const& v) {
  return dot(v, v);
}
    
template <typename T, unsigned int N>
inline T length(vector<T,N> const& v) {
  return sqrt(dot(v, v));
}
    
template<typename T>
inline auto cross(vector<T,3> const& v0, vector<T,3> const& v1) {
  return vector<T,3>{
    v0(1) * v1(2) - v0(2) * v1(1),
    v0(2) * v1(0) - v0(0) * v1(2),
    v0(0) * v1(1) - v0(1) * v1(0)
  };
}
    
template<typename T, unsigned int N>
inline auto normal(vector<T,N> const v) {
  return v / length(v);
}

// special case: quaternion
template<typename T>
class quaternion : public vector<T,4> {
public:
  quaternion() {}
  quaternion(std::initializer_list<T> l) : vector<T,4>(l) {};
};

template<typename T>
inline auto operator * (quaternion<T> const& a, quaternion<T> const& b) {
  return quaternion<T>{
    a(0) * b(0) - a(1) * b(1) - a(2) * b(2) - a(3) * b(3),
    a(2) * b(3) - a(3) * b(2) + a(0) * b(1) + a(1) * b(0),
    a(3) * b(1) - a(1) * b(3) + a(0) * b(2) + a(2) * b(0),
    a(1) * b(2) - a(2) * b(1) + a(0) * b(3) + a(3) * b(0)
  };
}

template<typename T>
inline auto from_euler(T const& a, T const& b, T const& c) {
  auto t = T(0.5);
  T const cx = cos(a * t);
  T const cy = cos(b * t);
  T const cz = cos(c * t);
  T const sx = sin(a * t);
  T const sy = sin(b * t);
  T const sz = sin(c * t);
  return quaternion<T>{
    cz * cy * cx + sz * sy * sx,
    cz * cy * sx - sz * sy * cx,
    cz * sy * cx + sz * cy * sx,
    sz * cy * cx - cz * sy * sx
  };
}

template<typename T>
inline auto from_axis(vector<T,3> const& axis, T const& angle) {
  T const c = cos(angle * T(0.5));
  T const s = sin(angle * T(0.5));
  auto n = s * normal(axis);
  return quaternion<T>{ c, n(0), n(1), n(2) };
}

template<typename T>
inline auto conjugate(quaternion<T> const& q) {
  return quaternion<T>{q(0), -q(1), -q(2), -q(3)};
}

template<typename T>
inline auto inverse(quaternion<T> const& q) {
  return conjugate(q) / dot(q, q);
}

template<typename T>
inline vector<T, 3> euler(quaternion<T> const& q) {
  T const sw = q(0) * q(0);
  T const sx = q(1) * q(1);
  T const sy = q(2) * q(2);
  T const sz = q(3) * q(3);
  return vector<T, 3>{atan2(T(2) * (q(2) * q(3) + q(0) * q(1)), sz - sy - sx + sw),
                      asin(T(2) * (q(0) * q(2) - q(1) * q(3))),
                      atan2(T(2) * (q(1) * q(2) + q(0) * q(3)), sx + sw - sz - sy)};
}
    
template<typename T>
inline auto axis(quaternion<T> const& q) {
  T const angle = acos(q(0));
  T const k = 1.0f / sin(angle);
  return vector<T, 3>{q(1) * k, q(2) * k, q(3) * k};
}
    
template<typename T>
inline auto angle(quaternion<T> const& q) {
  return acos(q(0)) * T(2);
}

template<typename T>
inline auto transform(quaternion<T> const& q, vector<T, 3> const& v) {
  quaternion<T> V{T(0), v(0), v(1), v(2)};
  quaternion<T> result = q * V * conjugate(q);
  return vector<T, 3>{result(0), result(1), result(2)};
}

template<typename T>
inline auto rotation(quaternion<T> const& q) {
  T const ww = q(0) * q(0);
  T const wx = q(0) * q(1);
  T const wy = q(0) * q(2);
  T const wz = q(0) * q(3);
  T const xx = q(1) * q(1);
  T const xy = q(1) * q(2);
  T const xz = q(1) * q(3);
  T const yy = q(2) * q(2);
  T const yz = q(2) * q(3);
  T const zz = q(3) * q(3);
  matrix<T, 4, 4> m;
  m(0, 0) = ww + xx - yy - zz;
  m(0, 1) = static_cast<T>(2) * (xy - wz);
  m(0, 2) = static_cast<T>(2) * (xz + wy);
  m(0, 3) = static_cast<T>(0);
            
  m(1, 0) = static_cast<T>(2) * (xy + wz);
  m(1, 1) = ww - xx + yy - zz;
  m(1, 2) = static_cast<T>(2) * (yz - wx);
  m(1, 3) = static_cast<T>(0);
            
  m(2, 0) = static_cast<T>(2) * (xz - wy);
  m(2, 1) = static_cast<T>(2) * (yz + wx);
  m(2, 2) = ww - xx - yy + zz;
  m(2, 3) = static_cast<T>(0);
            
  m(3, 0) = static_cast<T>(0);
  m(3, 1) = static_cast<T>(0);
  m(3, 2) = static_cast<T>(0);
  m(3, 3) = static_cast<T>(1);
  return m;
}

template<typename T>
inline auto translation(vector<T, 3> const& v) {
  auto m = identity<T, 4, 4>();
  m(0, 3) = v(0);
  m(1, 3) = v(1);
  m(2, 3) = v(2);
  return m;
}

template<typename T>
inline auto transformation(quaternion<T> const& q, vector<T, 3> const& v) {
  auto m = rotation(q);
  m(0, 3) = v(0);
  m(1, 3) = v(1);
  m(2, 3) = v(2);
  return m;
}

////////////////////////////////////////////////////////////////////////////////

// Assume row(3) to be (0,0,0,1)
template<typename T>
inline auto inverse_transformation(matrix<T, 4, 4> const& m) {
  T m00 =   m(1, 1) * m(2, 2)
    - m(2, 1) * m(1, 2);
  T m10 = - m(1, 0) * m(2, 2)
    + m(2, 0) * m(1, 2);
  T m20 =   m(1, 0) * m(2, 1)
    - m(2, 0) * m(1, 1);
        
  T m01 = - m(0, 1) * m(2, 2)
    + m(2, 1) * m(0, 2);
  T m11 =   m(0, 0) * m(2, 2)
    - m(2, 0) * m(0, 2);
  T m21 = - m(0, 0) * m(2, 1)
    + m(2, 0) * m(0, 1);

  T m02 =   m(0, 1) * m(1, 2)
    - m(1, 1) * m(0, 2);
  T m12 = - m(0, 0) * m(1, 2)
    + m(1, 0) * m(0, 2);
  T m22 =   m(0, 0) * m(1, 1)
    - m(1, 0) * m(0, 1);

  T m03 = - m(0, 1) * m(1, 2) * m(2, 3)
    + m(0, 1) * m(1, 3) * m(2, 2)
    + m(1, 1) * m(0, 2) * m(2, 3)
    - m(1, 1) * m(0, 3) * m(2, 2)
    - m(2, 1) * m(0, 2) * m(1, 3)
    + m(2, 1) * m(0, 3) * m(1, 2);
  T m13 =   m(0, 0) * m(1, 2) * m(2, 3)
    - m(0, 0) * m(1, 3) * m(2, 2)
    - m(1, 0) * m(0, 2) * m(2, 3)
    + m(1, 0) * m(0, 3) * m(2, 2)
    + m(2, 0) * m(0, 2) * m(1, 3)
    - m(2, 0) * m(0, 3) * m(1, 2);
  T m23 = - m(0, 0) * m(1, 1) * m(2, 3)
    + m(0, 0) * m(1, 3) * m(2, 1)
    + m(1, 0) * m(0, 1) * m(2, 3)
    - m(1, 0) * m(0, 3) * m(2, 1)
    - m(2, 0) * m(0, 1) * m(1, 3)
    + m(2, 0) * m(0, 3) * m(1, 1);

  T det = m(0, 0) * m00 + m(0, 1) * m10 + m(0, 2) * m20;
  return matrix<T, 4, 4>{
    m00, m01, m02, m03,
    m10, m11, m12, m13,
    m20, m21, m22, m23,
    0,   0,   0, det
  } / det;
}
    
template<typename T>
inline auto perspective_projection(T const& width, T const& height,
                                   T const& field_of_view,
                                   T const& znear, T const& zfar) {
  T const aspect = width / height;
  T const h = 1 / tan(field_of_view * pi / 360);
  T const zdiff = zfar - znear;
  T const a = -(zfar + znear) / zdiff;
  T const b = -2 * zfar * znear / zdiff;
  return matrix<T, 4, 4>{
    h / aspect, T(0),   T(0),  T(0),
    T(0),       h,      T(0),  T(0),
    T(0),       T(0),   a,     b,
    T(0),       T(0),   -T(1), T(0)
  };
}
template<typename T>
inline auto perspective_projection(T const& left, T const& right,
                                   T const& bottom, T const& top,
                                   T const& znear, T const& zfar) {
  T const a = 2 * znear / (right - left);
  T const b = (right + left) / (right - left);
  T const c = 2 * znear / (top - bottom);
  T const d = (top + bottom) / (top - bottom);
  T const e = -(zfar + znear) / (zfar - znear);
  T const f = -2 * zfar*znear / (zfar - znear);
  return matrix<T, 4, 4>{
    a,    T(0), b,     T(0),
    T(0), c,    d,     T(0),
    T(0), T(0), e,     f,
    T(0), T(0), -T(1), T(0)
  };
}
template<typename T>
inline auto orthogonal_projection(T const& left, T const& right,
                                  T const& bottom, T const& top,
                                  T const& znear, T const& zfar) {
  T const a = 2 / (right - left);
  T const b = -(right + left) / (right - left);
  T const c = 2 / (top - bottom);
  T const d = -(top + bottom) / (top - bottom);
  T const e = -2 / (zfar - znear);
  T const f = -(zfar + znear) / (zfar - znear);
  return matrix<T, 4, 4>{
    a,    T(0), T(0), b,
    T(0), c,    T(0), d,
    T(0), T(0), e,    f,
    T(0), T(0), T(0), T(1)
  };
}
template<typename T>
inline auto orthogonal_projection(T const& width, T const& height,
                                  T const& znear, T const& zfar) {
  return orthogonal_projection(-width/2, width/2, -height/2, height/2, znear, zfar);
}
template<typename T>
inline auto normal_matrix(matrix<T, 4, 4> const& mv) {
  return sub<3, 3>(transpose(inverse(mv)), 0, 0);
}

template<typename T>
inline auto look_at_matrix(vector<T, 3> const& eye,
                               vector<T, 3> const& at,
                               vector<T, 3> const& up) {
  vector<T, 3> zaxis = normal(at - eye);
  vector<T, 3> xaxis = normal(cross(up, zaxis));
  vector<T, 3> yaxis = cross(zaxis, xaxis);
  return matrix<T,4,4>{
    xaxis(0), xaxis(1), xaxis(2), -dot(xaxis, eye),
    yaxis(0), yaxis(1), yaxis(2), -dot(yaxis, eye),
    zaxis(0), zaxis(1), zaxis(2), -dot(zaxis, eye),
    T(0), T(0), T(0), T(1)
  };
}
    
template<typename T>
inline auto rotation_from_matrix(matrix<T, 4, 4> const& m) {
  quaternion<T> q;
  auto trace = m(0,0) + m(1,1) + m(2,2);
  if(trace > 0) {
    auto s = T(0.5) / sqrt(trace + T(1));
    q(0) = T(0.25) / s;
    q(1) = (m(2,1) - m(1,2)) * s;
    q(2) = (m(0,2) - m(2,0)) * s;
    q(3) = (m(1,0) - m(0,1)) * s;
  } else {
    if (m(0,0) > m(1,1) && m(0,0) > m(2,2)) {
      auto s = T(2) * sqrtf( 1.0f + m(0,0) - m(1,1) - m(2,2));
      q(0) = (m(2,1) - m(1,2)) / s;
      q(1) = T(0.25) * s;
      q(2) = (m(0,1) + m(1,0)) / s;
      q(3) = (m(0,2) + m(2,0)) / s;
    } else if (m(1,1) > m(2,2)) {
      auto s = T(2) * sqrtf( 1.0f + m(1,1) - m(0,0) - m(2,2));
      q(0) = (m(0,2) - m(2,0)) / s;
      q(1) = (m(0,1) + m(1,0)) / s;
      q(2) = T(0.25) * s;
      q(3) = (m(1,2) + m(2,1)) / s;
    } else {
      auto s = T(2) * sqrtf( 1.0f + m(2,2) - m(0,0) - m(1,1) );
      q(0) = (m(1,0) - m(0,1)) / s;
      q(1) = (m(0,2) + m(2,0)) / s;
      q(2) = (m(1,2) + m(2,1)) / s;
      q(3) = T(0.25) * s;
    }
  }
  return q;
}
    
template<typename T>
inline auto translation_from_matrix(matrix<T,4,4> const& m) {
  return vector<T,3>(m(0,3), m(1,3), m(2,3));
}
    
template<typename T>
inline auto transform_point(matrix<T,4,4> const& m, vector<T,3> const& v) {
  auto t = m * vector<T,4>{v(0), v(1), v(2), T(1)};
  return vector<T,3>(t(0), t(1), t(2));
}
    
template<typename T>
inline auto transform_vector(matrix<T,4,4> const& m, vector<T, 3> const& v) {
  auto t = m * vector<T,4>{v(0), v(1), v(2), T(0)};
  return vector<T, 3>(t(0), t(1), t(2));
}

} // namespace mth

#endif // COMMON_MTH_HPP
