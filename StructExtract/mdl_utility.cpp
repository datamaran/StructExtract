#include "mdl_utility.h"
#include "logger.h"

#include <map>
#include <string>
#include <algorithm>
#include <climits>
#include <cmath>

double CheckEnum(const std::vector<std::string>& attr_vec) {
    // Value to count mapping
    std::map<std::string, int> dict;
    int outlier_cnt = 0;
    double outlier_mdl = 0;

    for (const auto& attr : attr_vec)
        if (dict.count(attr) == 0)
            dict[attr] = 1;
        else
            ++dict[attr];

    int dictionary_size = 0;
    std::vector<int> freq;
    for (const auto& pair : dict)
        if (pair.second > (int)attr_vec.size() / 20 && pair.first.length() < 40) {
            freq.push_back(pair.second);
            dictionary_size += (pair.first.length() + 1) * 8;
        } else {
            outlier_mdl += (pair.first.length() + 1) * 8 * pair.second;
            outlier_cnt += pair.second;
        }
    freq.push_back(outlier_cnt);

    return FrequencyToMDL(freq) + outlier_mdl + dictionary_size;
}

bool ParseInt(const std::string& attr, int* v) {
    // Exclude prefix/suffix spaces
    int last = -1, first = attr.length();
    for (int j = 0; j < (int)attr.length(); ++j) {
        if (attr[j] != ' ') {
            last = std::max(last, j);
            first = std::min(first, j);
        }
    }

    if (last - first > 8) return false;
    int val = 0;
    // Missing value is treated as 0 (last < first)
    if (last >= first) {
        for (int j = first; j <= last; ++j)
            if (attr[j] >= '0' && attr[j] <= '9')
                val = val * 10 + attr[j] - '0';
            else
                return false;
    }
    *v = val;
    return true;
}

double CheckInt(const std::vector<std::string>& attr_vec) {
    // Minimum/Maximum Value
    int min_int_value = INT_MAX, max_int_value = -INT_MAX;
    double outlier_mdl = 0;

    for (const auto& attr : attr_vec) {
        int val;
        if (ParseInt(attr, &val)) {
            min_int_value = std::min(min_int_value, val);
            max_int_value = std::max(max_int_value, val);
        }
        else
            outlier_mdl += (attr.length() + 1) * 8;
    }

    // Compute MDL for integer
    return attr_vec.size() * log2(max_int_value - min_int_value + 1) + outlier_mdl;
}

bool ParseDouble(const std::string& attr, double* v, int *precision) {
    // Exclude prefix/suffix spaces
    int last = -1, first = attr.length();
    for (int j = 0; j < (int)attr.length(); ++j) {
        if (attr[j] != ' ') {
            last = std::max(last, j);
            first = std::min(first, j);
        }
    }
    double integer = 0, fractional = 0, dot_cnt = 0;
    *precision = 0;
    // if value is missing (last < first), treat the value as zero
    for (int j = first; j <= last; ++j) {
        if (attr[j] == '.')
            ++dot_cnt;
        else if (attr[j] >= '0' && attr[j] <= '9') {
            if (dot_cnt == 1) {
                (*precision)++;
                fractional = fractional * 10 + attr[j] - '0';
            }
            else
                integer = integer * 10 + attr[j] - '0';
        }
        else
            return false;
    }
    if (dot_cnt > 1)
        return false;
    *v = integer + fractional / pow(10, *precision);
    return true;
}

double CheckDouble(const std::vector<std::string>& attr_vec) {
    // Range and Precision
    double min_double = 1e10, max_double = -1e10;
    int double_precision = 0;
    double outlier_mdl = 0;

    for (const auto& attr : attr_vec) {
        double val;
        int precision;
        if (ParseDouble(attr, &val, &precision)) {
            min_double = std::min(min_double, val);
            max_double = std::max(max_double, val);
            double_precision = std::max(precision, double_precision);
        }
        else
            outlier_mdl += (attr.length() + 1) * 8;
    }

    double range = max_double - min_double + pow(10, -double_precision);
    if (range < 0) return 1e10;
    return attr_vec.size() * (log2(range) + double_precision * log2(10)) + outlier_mdl;
}

double CheckNormal(const std::vector<std::string>& attr_vec) {
    // Sum and Variance
    double mean = 0, variance = 0;
    int count = 0;
    int double_precision = 0;
    double outlier_mdl = 0;

    for (const auto& attr : attr_vec) {
        double val;
        int precision;
        if (ParseDouble(attr, &val, &precision)) {
            mean += val;
            variance += val * val;
            count++;
            double_precision = std::max(precision, double_precision);
        }
        else
            outlier_mdl += (attr.length() + 1) * 8;
    }
    mean /= count;
    variance = variance / count - mean * mean;
    if (variance < pow(10, -2 * double_precision))
        variance = pow(10, -2 * double_precision);

    if (count == 0) return 1e10;
    return attr_vec.size() * (log2(sqrt(2 * 3.1416 * variance)) + log2(exp(1)) * 0.5 + double_precision * log2(10)) + outlier_mdl;
}

double CheckFixedLength(const std::vector<std::string>& attr_vec) {
    // Positional Char Frequency
    std::vector<std::map<char, int>> fix_length_freq;
    int fix_length = -1;
    double outlier_mdl = 0;

    for (const auto& attr : attr_vec) {
        if (fix_length == -1) {
            fix_length = attr.length();
            fix_length_freq = std::vector<std::map<char, int>>(fix_length);
        }
        if (fix_length == (int)attr.length()) {
            for (int j = 0; j < (int)attr.length(); ++j)
                ++fix_length_freq[j][attr[j]];
        }
        else
            outlier_mdl += (attr.length() + 1) * 8;
    }

    double mdl = 0;
    for (int j = 0; j < fix_length; ++j) {
        std::vector<int> freq;
        for (const auto& pair : fix_length_freq[j])
            freq.push_back(pair.second);
        mdl += FrequencyToMDL(freq);
    }
    return mdl + outlier_mdl;
}

double CheckArbitraryLength(const std::vector<std::string>& attr_vec) {
    // Global Char Frequency
    std::map<char, int> char_freq;

    for (const auto& attr : attr_vec) {
        for (int j = 0; j < (int)attr.length(); ++j)
            ++char_freq[attr[j]];
        ++char_freq[0];
    }

    std::vector<int> freq;
    for (const auto& pair : char_freq)
        freq.push_back(pair.second);
    return FrequencyToMDL(freq);
}

double CheckTrivial(const std::vector<std::string>& attr_vec) {
    double ret = 0;
    for (const auto& attr : attr_vec)
        ret += (attr.length() + 1) * 8;
    return ret;
}

double FrequencyToMDL(const std::vector<int>& vec) {
    double sum = 0, total = 0;
    for (int num : vec) {
        if (num == 0) continue;
        sum += num * log2(num);
        total += num;
    }
    if (total == 0) return 0;
    return total * log2(total) - sum;
}

double TrivialMDL(const ParsedTuple* tuple) {
    if (tuple->is_empty) return 8;
    if (tuple->is_field) return tuple->value.length() * 8;
    double mdl = 0;
    for (const auto& child : tuple->attr)
        mdl += TrivialMDL(child.get());
    return mdl;
}
