#include "../include/fraction.h"

int fraction::sign() const noexcept
{
    return _denominator.sign();
}

big_integer fraction::nod(big_integer a, big_integer b) const
{
    if (a < b) std::swap(a, b);
    while (b != big_integer("0"))
    {
        a %= b;
		std::swap(a, b);
    }
    return a;
}

/*
    Функция для сокращения дроби
*/
void fraction::fraction_reducing()
{
    if (_denominator == big_integer("1")) return;
    bool is_negative = false;

    if (_denominator.sign() < 0)
    {
        _denominator.change_sign();
        is_negative = true;
    }

    big_integer gcd = _denominator, b = _numerator;

    if (gcd < b) std::swap(gcd, b);
    while (b != big_integer("0"))
    {
        big_integer temp = b;
        b = gcd % b;
        gcd = temp;
    }
    _numerator /= gcd;
    _denominator /= gcd;

    if (is_negative) _denominator.change_sign();
}

fraction::fraction(
    big_integer &&numerator,
    big_integer &&denominator):
        _numerator(std::forward<big_integer>(numerator)),
        _denominator(std::forward<big_integer>(denominator))
{
    if (_numerator.sign() < 0) throw std::logic_error("Sign should be in denominator!\n");
    if (_denominator == big_integer("0")) throw std::logic_error("Denominator should not be zero\n");

    fraction_reducing();
}

fraction::fraction(
    fraction const &other):
        _numerator(other._numerator),
        _denominator(other._denominator)
{}

fraction &fraction::operator=(
    fraction const &other)
{
    if (*this == other) return *this;
    _numerator = other._numerator;
    _denominator = other._denominator;
    return *this;
}

fraction::fraction(
    fraction &&other) noexcept:
        _numerator(std::move(other._numerator)),
        _denominator(std::move(other._denominator))
{}

fraction &fraction::operator=(
    fraction &&other) noexcept
{
    if (*this == other) return *this;
    _numerator = std::move(other._numerator);
    _denominator = std::move(other._denominator);
    return *this;
}

void fraction::make_same_denominator(fraction &a, fraction &b) const
{
    big_integer nok = (a._denominator * b._denominator) / nod(a._denominator, b._denominator);

    big_integer mult1 = nok / a._denominator;
    big_integer mult2 = nok / b._denominator;

    a._numerator *= mult1;
    a._denominator *= mult1;

    b._numerator *= mult2;
    b._denominator *= mult2;
}

fraction &fraction::operator+=(
    fraction const &other)
{
    fraction c_other(other);

    if (sign() < 0 && other.sign() < 0) 
    {
        _denominator.change_sign();
        c_other._denominator.change_sign();
        *this += c_other;
        _denominator.change_sign();
        return *this;
    }

    if (sign() < 0 && other.sign() > 0) 
    {     
        _denominator.change_sign();
        fraction res = other - *this; 
        return (*this = res);
    } 

    if (other.sign() < 0 && sign() > 0) 
    {
        c_other._denominator.change_sign(); 
        fraction res = *this - c_other;
        return (*this = res);
    }

    make_same_denominator(*this, c_other);

    _numerator += c_other._numerator;

    fraction_reducing();
    return *this;
}

fraction fraction::operator+(
    fraction const &other) const
{
    return fraction(*this) += other;
}

fraction &fraction::operator-=(
    fraction const &other)
{
    fraction c_other(other);

    if (sign() < 0 && c_other.sign() < 0) 
    {
        c_other._denominator.change_sign();
        *this = c_other - *this; 
        return *this;
    } 

    if (sign() > 0 && c_other.sign() < 0) 
    {
        c_other._denominator.change_sign();
        *this += c_other;
        return *this;
    }

    if (sign() < 0 && c_other.sign() > 0) 
    {
        _denominator.change_sign();
        *this += c_other;
        _denominator.change_sign();
        return *this;
    }

    make_same_denominator(*this, c_other);

    _numerator -= c_other._numerator;

    if (_numerator.sign() == -1) 
    {
        _numerator *= big_integer(-1);
        _denominator *= big_integer(-1);
    }

    fraction_reducing();
    return *this;
}

fraction fraction::operator-(
    fraction const &other) const
{
    return fraction(*this) -= other;
}

fraction &fraction::operator*=(
    fraction const &other)
{
    _numerator *= other._numerator;
    _denominator *= other._denominator;
    fraction_reducing();
    return *this;
}

fraction fraction::operator*(
    fraction const &other) const
{
    return fraction(*this) *= other;
}

fraction &fraction::operator/=(
    fraction const &other)
{
    _numerator *= other._denominator;
    _denominator *= other._numerator;

    fraction_reducing();
    return *this;
}

fraction fraction::operator/(
    fraction const &other) const
{
    return fraction(*this) /= other;
}

bool fraction::operator==(
    fraction const &other) const
{
    return (_numerator == other._numerator && _denominator == other._denominator);
}

bool fraction::operator!=(
    fraction const &other) const
{
    return !(*this == other);
}

bool fraction::operator>=(
    fraction const &other) const
{
    return !(*this < other);
}

bool fraction::operator>(
    fraction const &other) const
{
    if (sign() < 0 && other.sign() < 0) return (*this < other);

    if (sign() != other.sign()) return (sign() > 0);

    fraction c_this = *this;
    fraction c_other = other;

    make_same_denominator(c_this, c_other);

    return (c_this._numerator > c_other._numerator);
}

bool fraction::operator<=(
    fraction const &other) const
{
    return !(*this > other);
}

bool fraction::operator<(
    fraction const &other) const
{
    if (sign() < 0 && other.sign() < 0) return (abs() > other.abs());
    if (sign() != other.sign()) return (sign() < 0);

    fraction c_this = *this;
    fraction c_other = other;

    make_same_denominator(c_this, c_other);

    return (c_this._numerator < c_other._numerator);
}

std::ostream &operator<<(
    std::ostream &stream,
    fraction const &obj)
{
    big_integer denominator_copy = obj._denominator;
    if (denominator_copy.sign() < 0)
    {
        stream << "-";
        denominator_copy.change_sign();
    }
    stream << obj._numerator << "/" << denominator_copy;
    return stream;
}

std::istream &operator>>(
    std::istream &stream,
    fraction &obj)
{
    stream >> obj._numerator;
    char slash;
    if (slash != '/') throw std::logic_error("Unexpected symbol\n");
    stream >> obj._denominator;
    if (obj._numerator.sign() < 0)
    {
        obj._numerator.change_sign();
        obj._denominator.change_sign();
    }
    return stream;
}

bool fraction::is_zero() const
{
    return _numerator.is_zero();
}

int fraction::is_valid_eps(fraction const &eps) const noexcept
{
    return !((eps.sign() == -1) || eps.is_zero());
}

fraction fraction::abs() const
{
    if (sign() > 0) return *this;
    fraction tmp(*this);
    tmp._denominator.change_sign();
    return tmp;
}

fraction fraction::sin(
    fraction const &epsilon) const
{
    if (!is_valid_eps(epsilon)) throw std::logic_error("Invalid epsilon\n");

    fraction term = *this;
    fraction result(big_integer("0"), big_integer("1"));
    int n = 1;
    while (term.abs() >= epsilon) 
    {
        result += term;
        fraction minus_term = term;
        minus_term._denominator.change_sign();
        term = (minus_term) * (*this) * (*this) / fraction(big_integer(std::to_string((2 * n ) * (2 * n + 1))), big_integer("1"));
        n++;
    }

    return result;
}

fraction fraction::cos(
    fraction const &epsilon) const
{
    if (!is_valid_eps(epsilon)) throw std::logic_error("invalid epsilon");

    fraction term(big_integer("1"), big_integer("1"));
    fraction result = term;
    int n = 1;

    while (term.abs() >= epsilon) 
    {
        fraction minus_term = term;
        term._denominator.change_sign();
        term = minus_term * (*this) * (*this) / fraction(big_integer(std::to_string((n) * (n + 1))), big_integer("1"));
        result += term;
        n += 2;
    }

  return result;
}

fraction fraction::tg(
    fraction const &epsilon) const
{
	return sin(epsilon) / cos(epsilon);
}

fraction fraction::ctg(
    fraction const &epsilon) const
{
    return cos(epsilon) / sin(epsilon);
}

fraction fraction::sec(
    fraction const &epsilon) const
{
    // 1 / cos(x)
    return fraction(big_integer("1"), big_integer("1")) / cos(epsilon);
}

fraction fraction::cosec(
    fraction const &epsilon) const
{
    // 1 / sin(x)
    return fraction(big_integer("1"), big_integer("1")) / sin(epsilon);
}

fraction fraction::arcsin(
    fraction const &epsilon) const
{
    if (*this < fraction(big_integer("1"), big_integer("-1")) || *this > fraction(big_integer("1"), big_integer("1"))) 
        throw std::logic_error("Invalid range\n");
    
    fraction result = *this;
    fraction term = *this;
    int n = 1;

    while (term.abs() > epsilon) 
    {
        term *= fraction(big_integer(std::to_string(n)), big_integer("1"));
        term = term * (*this) * (*this) * fraction(big_integer(std::to_string(n)), big_integer("1")) / fraction(big_integer(std::to_string(n + 1)), big_integer("1")) / fraction(big_integer(std::to_string(n + 2)), big_integer("1"));
        result += term;
        n += 2;
    }

    return result;
}

fraction fraction::arccos(
    fraction const &epsilon) const
{
    if (*this < fraction(big_integer("1"), big_integer("-1")) || *this > fraction(big_integer("1"), big_integer("1"))) 
        throw std::logic_error("Invalid range\n");
    
    fraction tmp = fraction(big_integer("1"), big_integer("1")).arcsin(epsilon);
    fraction result = tmp - arcsin(epsilon);
    return result;
}

fraction fraction::arctg(
    fraction const &epsilon) const
{
    if (*this < fraction(big_integer("1"), big_integer("-1")) || *this > fraction(big_integer("1"), big_integer("1"))) 
        throw std::logic_error("Invalid range\n");

    fraction tmp = ((*this) / (pow(2) + fraction(big_integer("1"), big_integer("1"))).root(2, epsilon)).arcsin(epsilon);
}

fraction fraction::arcctg(
    fraction const &epsilon) const
{
    if (*this < fraction(big_integer("1"), big_integer("-1")) || *this > fraction(big_integer("1"), big_integer("1"))) 
        throw std::logic_error("Invalid range\n");

    fraction tmp = (*this) * (fraction(big_integer("1"), big_integer("1")) / ((*this) * (*this) + fraction(big_integer("1"), big_integer("1"))).root(2, epsilon));
    return tmp.arccos(epsilon);
}

fraction fraction::arcsec(
    fraction const &epsilon) const
{
    return (fraction(big_integer("1"), big_integer("1")) / (*this)).arccos(epsilon);
}

fraction fraction::arccosec(
    fraction const &epsilon) const
{
    return (fraction(big_integer("1"), big_integer("1")) / (*this)).arcsin(epsilon);
}

fraction fraction::pow(
    size_t degree) const
{
    if (degree == 0) return fraction(big_integer("1"), big_integer("1")); 
    if (degree < 0) return fraction(big_integer("1"), big_integer("1")) / pow(-degree);

    fraction base(*this);
    fraction result(big_integer(1), big_integer(1));

    while (degree > 0) 
    {
        if (degree % 2 == 1) 
        {
            result *= base;
            degree--;
        }
        base *= base;
        degree /= 2; 
    }

    return result;
}

fraction fraction::root(
    size_t degree,
    fraction const &epsilon) const
{
    fraction left(big_integer("0"), big_integer("1"));
    fraction right = *this;
    fraction mid(big_integer("1"), big_integer("1"));

    while (right - left > epsilon) 
    {
        mid = (left + right) / fraction(big_integer("2"), big_integer("1"));
        fraction pow_mid = mid.pow(degree);

        if (pow_mid < *this) left = mid;
        else right = mid;
    }

    return (left + right) / fraction(big_integer("2"), big_integer("1"));
}

fraction fraction::log2(
    fraction const &epsilon) const
{
    return (ln(epsilon) / fraction(big_integer("2"), big_integer("1")).ln(epsilon));
}

fraction fraction::ln(
    fraction const &epsilon) const
{
    if (*this <= fraction(big_integer("0"), big_integer("1")))
        throw std::logic_error("Cannot compute ln of negative value\n");
    
    fraction log_value(big_integer("0"), big_integer("1"));
    fraction x = (*this - fraction(big_integer("1"), big_integer("1"))) / (*this + fraction(big_integer("1"), big_integer("1")));
    int n = 1;

    fraction term(x);
    fraction x_pow = x;

    while (term.abs() > epsilon)
    {
        term = x_pow / fraction(big_integer(std::to_string(2 * n - 1)), big_integer("1"));
        log_value += term;
        x_pow *= x.pow(2);
        n++;
    }

    return log_value * fraction(big_integer("2"), big_integer("1"));
}

fraction fraction::lg(
    fraction const &epsilon) const
{
    return (ln(epsilon) / fraction(big_integer("10"), big_integer("1")).ln(epsilon));
}
