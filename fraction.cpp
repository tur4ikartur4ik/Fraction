#include <iostream>
#include <vector>
#include <map>
#include <fstream>
#include <gmp.h>

using namespace std;

class Fraction
{
private:
    mpz_t numerator;
    mpz_t denominator;

public:
    
    // Конструктор по умолчанию
    Fraction()
    {
        // mpz_init(x): инициализируем большое целое x
        mpz_init(numerator);
        mpz_init(denominator);
    }

    // Конструктор с параметрами
    Fraction(const char* num, const char* den)
    {
        // mpz_init(x): инициализируем большое целое x
        mpz_init(numerator);
        mpz_init(denominator);

        Set(num, den);
    }

    // Установка значений числителю и знаменателю
    void Set(const char* num, const char* den)
    {
        // mpz_set_str(a, a_, base): числу a присваиваем значение a_ в системе счисления base,        
        // вернёт 0, если данные успешно считались (данные: "целое число")
        if (!num || !den || num[0] == '\0' || den[0] == '\0' || // Проверка на пустые строки
            mpz_set_str(numerator, num, 10) != 0 || mpz_set_str(denominator, den, 10) != 0) // Проверка на некорректные данные
        {
            // mpz_clear(x): освобождает память, выделенную под x
            mpz_clear(numerator);
            mpz_clear(denominator);
            throw std::invalid_argument("Некорректный ввод данных.");
        }

        // mpz_cmp_ui(a1, a2): сравнение a1 с a2. Если равны, вернёт 0
        if (mpz_cmp_ui(denominator, 0) == 0)
        {
            mpz_clear(numerator);
            mpz_clear(denominator);
            throw std::invalid_argument("Знаменатель не может быть равен 0.");
        }

        // Перенесём минус из знаменателя в числитель. Или сократим минусы
        if (mpz_sgn(denominator) < 0) // Если знаменатель отрицательный, то 
        {
            // mpz_neg(a, b): a = -b
            mpz_neg(numerator, numerator);      // Меняем знак числителя на противоположный
            mpz_neg(denominator, denominator);  // Меняем знак знаменателя на противоположный
        }
    }

    // Сокращение дроби
    void reduce()
    {
        mpz_t gcd;
        mpz_init(gcd);

        // mpz_gcd(a, b, c): a = НОД(b, c)
        mpz_gcd(gcd, numerator, denominator);

        // mpz_divexact(a, b, c): a = b / c
        // использовать можно, когда деление выполняется без остатка
        mpz_divexact(numerator, numerator, gcd);
        mpz_divexact(denominator, denominator, gcd);

        mpz_clear(gcd);
    }

    // Формирование десятичной дроби
    string toDecimal(size_t maxDigits = 26) const
    {
        // Если числитель равен 0, то дробь равна 0
        // mpz_cmp_ui(a, b): сравнивает большое число a с обычным числом b
        if (mpz_cmp_ui(numerator, 0) == 0)
            return "0";

        mpz_t numAbs;       // Mодуль числителя
        mpz_t denAbs;       // Mодуль знаменателя
        mpz_t integerPart;  // Целая часть дроби
        mpz_t remainder;    // Остаток от деления
        mpz_t temp;         // Временная переменная: remainder * 10
        mpz_t digit;        // Очередная цифра после запятой

        mpz_init(numAbs);
        mpz_init(denAbs);
        mpz_init(integerPart);
        mpz_init(remainder);
        mpz_init(temp);
        mpz_init(digit);
        
        string result;      // сюда будем собирать весь ответ
        string fractional;  // сюда будем собирать цифры после запятой

        auto CLEAR = [&]()
        {
            mpz_clear(numAbs);
            mpz_clear(denAbs);
            mpz_clear(integerPart);
            mpz_clear(remainder);
            mpz_clear(temp);
            mpz_clear(digit);
        };

        // mpz_abs(a, b): a = |b|
        mpz_abs(numAbs, numerator);
        mpz_abs(denAbs, denominator);

        // mpz_tdiv_qr(a, b, c, d): a = c / d и b = c % d
        mpz_tdiv_qr(integerPart, remainder, numAbs, denAbs);

        // Если дробь отрицательная, сразу добавим знак "-"
        // mpz_sgn(a): определяет знак числа a
        if (mpz_sgn(numerator) < 0) result += "-";

        // Переводим целую часть в строку
        // mpz_sizeinbase(a, 10): количество цифр числа a в десятичной системе
        size_t len = mpz_sizeinbase(integerPart, 10) + 2;
        char* buffer = new char[len];

        // mpz_get_str(a, 10, b): переводит число b в десятичную строку a
        mpz_get_str(buffer, 10, integerPart);
        result += buffer;

        delete[] buffer;

        // Если остаток равен 0, значит дробь конечная и дробной части нет
        if (mpz_cmp_ui(remainder, 0) == 0)
        {
            CLEAR();
            return result;
        }

        // Добавляем десятичную точку
        result += ".";

        // В этой таблице будем хранить:
        // остаток -> позиция, где он впервые встретился в дробной части
        // Если остаток повторится, значит начался период
        map<string, size_t> seenRemainders;

        // Пока остаток не стал нулём, продолжаем деление в столбик
        while (mpz_cmp_ui(remainder, 0) != 0)
        {
            // Превращаем текущий остаток в строку, чтобы можно было положить в map
            len = mpz_sizeinbase(remainder, 10) + 2; // +2 - возможный знак - и '\0'
            char* remBuffer = new char[len];
            mpz_get_str(remBuffer, 10, remainder);

            string key = remBuffer;
            delete[] remBuffer;

            // Проверяем: встречался ли уже такой остаток раньше
            map<string, size_t>::iterator it = seenRemainders.find(key);

            if (it != seenRemainders.end())
            {
                // Если остаток повторился, то с позиции it->second начинается период
                fractional.insert(it->second, "(");
                fractional += ")";

                result += fractional;

                CLEAR();

                return result;
            }

            // Запоминаем, на какой позиции впервые встретился этот остаток
            seenRemainders[key] = fractional.size();

            // Дальше делаем один шаг деления в столбик:
            // умножаем остаток на 10, делим на знаменатель,
            // получаем новую цифру и новый остаток

            // mpz_mul_ui(a, b, c): a = b * c
            mpz_mul_ui(temp, remainder, 10);

            // mpz_tdiv_qr(a, b, c, d): a = c / d, b = c % d
            mpz_tdiv_qr(digit, remainder, temp, denAbs);

            // digit здесь всегда одна цифра от 0 до 9
            // mpz_get_ui(a): возвращает значение большого числа a как unsigned long
            fractional += char('0' + mpz_get_ui(digit));

            // Если цифр после запятой стало больше, чем лет счастливого брака моих родителей, обрываем вывод
            if (fractional.size() >= maxDigits)
            {
                result += fractional + "...";
                CLEAR();
                return result;
            }
        }

        // Если вышли из цикла, значит остаток стал 0, дробь конечная
        result += fractional;

        CLEAR();

        return result;
    }

    // Перегрузка << для вывода на экран и в файл
    friend ostream& operator<<(ostream& out, const Fraction& f)
    {
        size_t len1 = mpz_sizeinbase(f.numerator, 10) + 2;
        size_t len2 = mpz_sizeinbase(f.denominator, 10) + 2;

        char* numStr = new char[len1];
        char* denStr = new char[len2];

        // mpz_get_str(a, 10, b): переводит большое число b в десятичную строку a
        mpz_get_str(numStr, 10, f.numerator);
        mpz_get_str(denStr, 10, f.denominator);

        out << numStr << "/" << denStr;

        delete[] numStr;
        delete[] denStr;

        return out;
    }

    // Оператор присваивания
    Fraction& operator=(const Fraction& other)
    {
        if (this != &other)
        {
            // mpz_set(a, b): a = b
            mpz_set(numerator, other.numerator);
            mpz_set(denominator, other.denominator);
        }

        return *this;
    }

    // Перегрузка + : a/b + c/d = (ad + bc) / (bd)
    Fraction operator+(const Fraction& other) const
    {
        Fraction result("0", "1");

        mpz_t temp;
        mpz_init(temp);

        // mpz_mul(a, b, c): a = b * c
        mpz_mul(result.numerator, numerator, other.denominator);
        
        mpz_mul(temp, other.numerator, denominator);

        // mpz_add(a, b, c): a = b + c
        mpz_add(result.numerator, result.numerator, temp);

        mpz_mul(result.denominator, denominator, other.denominator);

        mpz_clear(temp);

        result.reduce();
        return result;
    }

    // Перегрузка - : a/b - c/d = (ad - bc) / (bd)
    Fraction operator-(const Fraction& other) const
    {
        Fraction result("0", "1");

        mpz_t temp;
        mpz_init(temp);

        // mpz_mul(a, b, c): a = b * c
        mpz_mul(result.numerator, numerator, other.denominator);

        mpz_mul(temp, other.numerator, denominator);

        // mpz_sub(a, b, c): a = b - c
        mpz_sub(result.numerator, result.numerator, temp);

        mpz_mul(result.denominator, denominator, other.denominator);

        mpz_clear(temp);

        result.reduce();
        return result;
    }

    // Перегрузка * : a/b * c/d = (ac) / (bd)
    Fraction operator*(const Fraction& other) const
    {
        Fraction result("0", "1");

        // mpz_mul(a, b, c): a = b * c
        mpz_mul(result.numerator, numerator, other.numerator);
        mpz_mul(result.denominator, denominator, other.denominator);

        result.reduce();
        return result;
    }

    // Перегрузка / : (a/b) / (c/d) = (a/b) * (d/c)
    Fraction operator/(const Fraction& other) const
    {
        // Если чисдитель дроби справа = 0, то грустим
        if (mpz_cmp_ui(other.numerator, 0) == 0)
            throw invalid_argument("Деление на ноль невозможно.");

        Fraction result("0", "1");

        // mpz_mul(a, b, c): a = b * c
        mpz_mul(result.numerator, numerator, other.denominator);
        mpz_mul(result.denominator, denominator, other.numerator);

        // Если после деления знаменатель стал отрицательным, переносим минус в числитель
        // mpz_sgn(a): определяет знак числа a
        if (mpz_sgn(result.denominator) < 0)
        {
            // mpz_neg(a, b): a = -b
            mpz_neg(result.numerator, result.numerator);
            mpz_neg(result.denominator, result.denominator);
        }

        result.reduce();
        return result;
    }

    ~Fraction()
    {
        mpz_clear(numerator);
        mpz_clear(denominator);
    }

};



int main()
{    
    // Для арифмитических операций
    Fraction fract1("15", "22");
    Fraction fract2;
    fract2.Set("24", "14");

    Fraction sum;   sum = fract1 + fract1;
    Fraction diff;  diff = fract1 - fract2;
    Fraction prod;  prod = fract1 * fract2;
    Fraction quot;  quot = fract1 / fract2;

    ofstream fout("fout.txt");

    cout << "fract1 = " << fract1 << " = " << fract1.toDecimal() << "\n";
    fout << "fract1 = " << fract1 << " = " << fract1.toDecimal() << "\n";

    cout << "fract2 = " << fract2 << " = "; 
    fout << "fract2 = " << fract2 << " = "; 
    fract2.reduce(); // Сокращение
    cout << fract2 << " = " << fract2.toDecimal() << 
        " = <Установим 2 знака после запятой> = " << fract2.toDecimal(2) << "\n";
    fout << fract2 << " = " << fract2.toDecimal() <<
        " = <Установим 2 знака после запятой> = " << fract2.toDecimal(2) << "\n";

    cout << "\n"; fout << "\n"; 

    cout << "sum  = fract1 + fract2 = " << fract1 << " + " << fract2 << " = " << sum << "\n";
    fout << "sum  = fract1 + fract2 = " << fract1 << " + " << fract2 << " = " << sum << "\n";

    cout << "diff = fract1 - fract2 = " << fract1 << " - " << fract2 << " = " << diff << "\n";
    fout << "diff = fract1 - fract2 = " << fract1 << " - " << fract2 << " = " << diff << "\n";

    cout << "prod = fract1 * fract2 = " << fract1 << " * " << fract2 << " = " << prod << "\n";
    fout << "prod = fract1 * fract2 = " << fract1 << " * " << fract2 << " = " << prod << "\n";

    cout << "quot = fract1 / fract2 = " << fract1 << " / " << fract2 << " = " << quot << "\n";
    fout << "quot = fract1 / fract2 = " << fract1 << " / " << fract2 << " = " << quot << "\n"; 
    
    cout << "\n"; fout << "\n"; 
    return 0;
}
