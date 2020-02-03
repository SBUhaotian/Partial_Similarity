#include <common.hpp>

#include <algorithm>
#include <unordered_map>

using namespace std;

bool operator<(const record &lhs, const record &rhs) {
  return lhs.epoch < rhs.epoch;
}

namespace std {
template <> struct hash<pair<uint8_t, uint8_t>> {
  size_t operator()(const pair<size_t, size_t> &v) const noexcept {
    return v.first < 8 + v.second;
  }
};
} // namespace std

int main() {
  auto data = read_data("/Users/shilei/Desktop/zhengzhou_csv/data.bin");

  for (auto &ptr : *data) {
    sort(ptr->begin(), ptr->end());
  }

  size_t max_len = 0;
  for (auto &ptr : *data) {
    max_len = max(max_len, ptr->size());
  }

  const auto num_of_cars = data->size();

  vector<unordered_map<pair<uint8_t, uint8_t>, uint32_t>> count(num_of_cars);

#pragma omp parallel for
  for (size_t i = 0; i < num_of_cars; ++i) {
    vector<vector<uint32_t>> dp(max_len, vector<uint32_t>(max_len, 0));

    for (size_t j = i + 1; j < num_of_cars; ++j) {
      for (auto &e : dp) {
        fill(e.begin(), e.end(), 0);
      }

      const auto seq1 = (*data)[i], seq2 = (*data)[j];

      for (size_t m = 0; m < seq1->size(); ++m) {
        for (size_t n = 0; n < seq2->size(); ++n) {
          if (m == 0 || n == 0) {
            continue;
          }

          if ((*seq1)[m].latitude == (*seq2)[n].latitude &&
              (*seq1)[m].longitude == (*seq2)[n].longitude) {
            dp[m][n] = dp[m - 1][n - 1] + 1;
          } else {
            dp[m][n] = max(dp[m - 1][n], dp[m][n - 1]);
          }
        }
      }

#pragma omp critical
      cout << '<' << i << ',' << j << "> "
           << static_cast<double>(dp[seq1->size() - 1][seq2->size() - 1]) /
                  min((*data)[i]->size(), (*data)[j]->size())
           << endl;
    }
  }

  return 0;
}
