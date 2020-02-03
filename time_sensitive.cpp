#include <common.hpp>

#include <algorithm>
#include <iostream>
#include <unordered_map>

using namespace std;

bool operator<(const record &lhs, const record &rhs) {
  return lhs.epoch < rhs.epoch;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    cerr << "usage: " << argv[0] << " data_file [sample probability]" << endl;
    return 1;
  }

  string file_name = argv[1];

  auto data_ptr = read_data(file_name);

  if (!data_ptr) {
    return 1;
  }

  if (argc >= 3) {
    data_ptr = sample_data(data_ptr, stof(string(argv[2])));
  }

  auto &data = *data_ptr;

  const auto num_of_cars = data.size();

#pragma omp parallel for
  for (size_t i = 0; i < num_of_cars; ++i) {
    sort(data[i].begin(), data[i].end());
  }

#pragma omp parallel for
  for (size_t i = 0; i < num_of_cars; ++i) {
    for (size_t j = i + 1; j < num_of_cars; ++j) {
      const auto &data1 = data[i], &data2 = data[j];

      if (data1.empty() || data2.empty()) {
        cout << '<' << i << ',' << j << "> " << 0 << '\n';
        continue;
      }

      size_t count = 0;

      auto itr1 = data1.cbegin(), itr2 = data2.cbegin();

      while (itr1 != data1.cend() && itr2 != data2.cend()) {
        if (itr1->epoch < itr2->epoch) {
          ++itr1;
        } else if (itr1->epoch > itr2->epoch) {
          ++itr2;
        } else {
#ifndef WENZHOU
          if (static_cast<int>(itr1->latitude) ==
                  static_cast<int>(itr2->latitude) &&
              static_cast<int>(itr1->longitude) ==
                  static_cast<int>(itr2->longitude)) {
#else
          if (itr1->latitude == itr2->latitude &&
              itr1->longitude == itr2->longitude) {
#endif
            ++count;
          }
          ++itr1;
          ++itr2;
        }
      }

#pragma omp critical
      cout << '<' << i << ',' << j << "> "
           << static_cast<float>(count) / min(data[i].size(), data2.size())
           << '\n';
    }
  }

  return 0;
}
