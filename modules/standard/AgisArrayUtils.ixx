export module AgisArrayUtils;

import <vector>;

export namespace Agis
{

std::vector<long long>
    sorted_union(const std::vector<long long>& vec1, const std::vector<long long>& vec2) {
    std::vector<long long> result;
    int i = 0;
    int j = 0;

    while (i < vec1.size() && j < vec2.size()) {
        if (vec1[i] < vec2[j]) {
            if (result.empty() || vec1[i] != result.back()) {
                result.push_back(vec1[i]);
            }
            i++;
        }
        else if (vec1[i] > vec2[j]) {
            if (result.empty() || vec2[j] != result.back()) {
                result.push_back(vec2[j]);
            }
            j++;
        }
        else {
            if (result.empty() || vec1[i] != result.back()) {
                result.push_back(vec1[i]);
            }
            i++;
            j++;
        }
    }

    // If there are any remaining elements in either vector, add them to the result.
    while (i < vec1.size()) {
        if (result.empty() || vec1[i] != result.back()) {
            result.push_back(vec1[i]);
        }
        i++;
    }

    while (j < vec2.size()) {
        if (result.empty() || vec2[j] != result.back()) {
            result.push_back(vec2[j]);
        }
        j++;
    }

    return result;
}

}