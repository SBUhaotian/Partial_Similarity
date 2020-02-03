#include <common.hpp>

#include <algorithm>
#include <iostream>
#include <unordered_map>

using namespace std;

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

#ifndef WENZHOU
  vector<unordered_map<pair<size_t, size_t>, uint32_t>> count(num_of_cars);
#else
  vector<unordered_map<pair<float, float>, uint32_t>> count(num_of_cars);
#endif

  for (size_t i = 0; i < num_of_cars; ++i) {
    auto &map = count[i];
    for (const auto &rec : data[i]) {
#ifndef WENZHOU
      map[make_pair(static_cast<size_t>(rec.longitude),
                    static_cast<size_t>(rec.latitude))]++;
#else
      map[make_pair(rec.longitude, rec.latitude)]++;
#endif
    }
  }

#pragma omp parallel for
  for (size_t i = 0; i < num_of_cars; ++i) {
    for (size_t j = i + 1; j < num_of_cars; ++j) {
      size_t c = 0;

      for (const auto &p : count[i]) {
        if (count[j].find(p.first) != count[j].end()) {
          c += min(p.second, count[j][p.first]);
        }
      }

#pragma omp critical
      cout << '<' << i << ',' << j << "> "
           << static_cast<double>(c) / min(data[i].size(), data[j].size())
           << '\n';
    }
  }

  return 0;
}
