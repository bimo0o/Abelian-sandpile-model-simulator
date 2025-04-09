#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <filesystem>
#include <cstdint>
#include <limits>

template<typename T>
class DynamicArray {
 private:
  T *data;
  size_t size_;
  size_t capacity_;

 public:
  DynamicArray() : data(nullptr), size_(0), capacity_(0) {
  }

  ~DynamicArray() {
	delete[] data;
  }

  void push_back(const T &value) {
	if (size_ == capacity_) {
	  size_t new_capacity = (capacity_ == 0) ? 1 : capacity_ * 2;
	  T *new_data = new T[new_capacity];
	  for (size_t i = 0; i < size_; i++) {
		new_data[i] = data[i];
	  }
	  delete[] data;
	  data = new_data;
	  capacity_ = new_capacity;
	}
	data[size_++] = value;
  }

  [[nodiscard]] size_t size() const { return size_; }
  T &operator[](size_t index) { return data[index]; }
  const T &operator[](size_t index) const { return data[index]; }
};

struct Args_from_Terminal {
  int max_iter = 0;
  int freq = 0;
  std::string input;
  std::string output;
};

struct Cell {
  int16_t x;
  int16_t y;
  uint64_t sand;
};

void expandGrid(uint64_t **&grid, int16_t &width, int16_t &height) {
  for (int16_t y{}; y < height; ++y) {
	auto *new_row = new uint64_t[width + 2]();
	std::copy(grid[y], grid[y] + width, new_row + 1);
	delete[] grid[y];
	grid[y] = new_row;
  }

  auto **new_grid = new uint64_t *[height + 2];
  new_grid[0] = new uint64_t[width + 2]();
  std::copy(grid, grid + height, new_grid + 1);
  new_grid[height + 1] = new uint64_t[width + 2]();
  delete[] grid;
  grid = new_grid;

  width += 2;
  height += 2;
}

void saveToBMP(const std::string &filename, uint64_t **grid, int16_t width, int16_t height) {
  int minX = width;
  int maxX = 0;
  int minY = height;
  int maxY = 0;

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      if (grid[y][x] > 0) {
        if (x < minX) minX = x;
        if (x > maxX) maxX = x;
        if (y < minY) minY = y;
        if (y > maxY) maxY = y;
      }
    }
  }

  int newWidth = maxX - minX + 1;
  int newHeight = maxY - minY + 1;

  // Настройки BMP
  int bytesPerRow = (newWidth + 1) / 2;
  if (bytesPerRow % 4 != 0) {
    bytesPerRow += 4 - (bytesPerRow % 4);
  }

  int dataSize = bytesPerRow * newHeight;
  int fileSize = 54 + 64 + dataSize; // 54 - заголовок, 64 - палитра

  // Создаем заголовок BMP
  unsigned char header[54] = {
    'B', 'M',  // Сигнатура BMP
    (unsigned char)fileSize, (unsigned char)(fileSize >> 8),
    (unsigned char)(fileSize >> 16), (unsigned char)(fileSize >> 24),
    0, 0, 0, 0,  // Зарезервировано
    54 + 64, 0, 0, 0,  // Смещение до данных
    40, 0, 0, 0,  // Размер заголовка
    (unsigned char)newWidth, (unsigned char)(newWidth >> 8),
    (unsigned char)(newWidth >> 16), (unsigned char)(newWidth >> 24),
    (unsigned char)newHeight, (unsigned char)(newHeight >> 8),
    (unsigned char)(newHeight >> 16), (unsigned char)(newHeight >> 24),
    1, 0,  // Количество плоскостей
    4, 0,  // Бит на пиксель
    0, 0, 0, 0,  // Сжатие
    0, 0, 0, 0,  // Размер изображения
    0, 0, 0, 0,  // Горизонтальное разрешение
    0, 0, 0, 0,  // Вертикальное разрешение
    8, 0, 0, 0,  // Количество цветов
    8, 0, 0, 0   // Важные цвета
  };

  // Простая палитра: белый, зеленый, фиолетовый, желтый, черный
  unsigned char palette[20] = {
    255, 255, 255, 0,  // Белый
    0, 255, 0, 0,      // Зеленый
    128, 0, 128, 0,    // Фиолетовый
    0, 255, 255, 0,    // Желтый
    0, 0, 0, 0         // Черный
  };

  // Записываем файл
  std::ofstream file(filename, std::ios::binary);

  // Пишем заголовок и палитру
  file.write((char*)header, 54);
  file.write((char*)palette, 64);

  // Буфер для строки пикселей
  unsigned char* row = new unsigned char[bytesPerRow];

  // Пишем пиксели
  for (int y = newHeight - 1; y >= 0; y--) {
    // Очищаем буфер
    for (int i = 0; i < bytesPerRow; i++) {
      row[i] = 0;
    }

    // Заполняем строку
    for (int x = 0; x < newWidth; x += 2) {
      unsigned char pixel1 = grid[y + minY][x + minX];
      if (pixel1 > 3) pixel1 = 4;

      unsigned char pixel2 = 0;
      if (x + 1 < newWidth) {
        pixel2 = grid[y + minY][x + minX + 1];
        if (pixel2 > 3) pixel2 = 4;
      }

      row[x/2] = (pixel1 << 4) | pixel2;
    }

    // Записываем строку
    file.write((char*)row, bytesPerRow);
  }

  delete[] row;
  file.close();
}

bool isStable(uint64_t **grid, int16_t width, int16_t height) {
  for (int16_t y{}; y < height; ++y) {
	for (int16_t x{}; x < width; ++x) {
	  if (grid[y][x] >= 4) return false;
	}
  }
  return true;
}

void toppleByDivision(uint64_t **&grid, int16_t &width, int16_t &height, int16_t &min_x, int16_t &min_y) {
  bool needExpand{};
  auto **temp = new uint64_t *[height];
  for (int16_t i = 0; i < height; i++) {
	temp[i] = new uint64_t[width];
	std::memcpy(temp[i], grid[i], width * sizeof(uint64_t));
  }

  // Проверяем необходимость расширения
  for (int16_t y{}; y < height; ++y) {
	if (grid[y][0] >= 4 || grid[y][width - 1] >= 4) {
	  needExpand = true;
	  break;
	}
  }
  for (int16_t x{}; x < width; ++x) {
	if (grid[0][x] >= 4 || grid[height - 1][x] >= 4) {
	  needExpand = true;
	  break;
	}
  }

  if (needExpand) {
	expandGrid(grid, width, height);
	min_x--;
	min_y--;

	for (int16_t i = 0; i < height - 2; i++) {
	  delete[] temp[i];
	}
	delete[] temp;

	temp = new uint64_t *[height];
	for (int16_t i = 0; i < height; i++) {
	  temp[i] = new uint64_t[width];
	  std::memcpy(temp[i], grid[i], width * sizeof(uint64_t));
	}
  }

  for (int16_t y{}; y < height; ++y) {
	for (int16_t x{}; x < width; ++x) {
	  if (temp[y][x] >= 4) {
		uint64_t distribute = temp[y][x] / 4;
		grid[y][x] = temp[y][x] % 4;

		grid[y - 1][x] += distribute;
		grid[y + 1][x] += distribute;
		grid[y][x - 1] += distribute;
		grid[y][x + 1] += distribute;
	  }
	}
  }

  for (int16_t i = 0; i < height; i++) {
	delete[] temp[i];
  }
  delete[] temp;
}

void toppleBySubtraction(uint64_t **&grid, int16_t &width, int16_t &height, int16_t &min_x, int16_t &min_y) {
  bool needExpand{};
  uint64_t **temp = new uint64_t *[height];
  for (int16_t i = 0; i < height; i++) {
	temp[i] = new uint64_t[width];
	std::memcpy(temp[i], grid[i], width * sizeof(uint64_t));
  }

  for (int16_t y{}; y < height; ++y) {
	if (grid[y][0] >= 4 || grid[y][width - 1] >= 4) {
	  needExpand = true;
	  break;
	}
  }
  for (int16_t x{}; x < width; ++x) {
	if (grid[0][x] >= 4 || grid[height - 1][x] >= 4) {
	  needExpand = true;
	  break;
	}
  }

  if (needExpand) {
	expandGrid(grid, width, height);
	min_x--;
	min_y--;

	for (int16_t i = 0; i < height - 2; i++) {
	  delete[] temp[i];
	}
	delete[] temp;

	temp = new uint64_t *[height];
	for (int16_t i = 0; i < height; i++) {
	  temp[i] = new uint64_t[width];
	  std::memcpy(temp[i], grid[i], width * sizeof(uint64_t));
	}
  }

  for (int16_t y{}; y < height; ++y) {
	for (int16_t x{}; x < width; ++x) {
	  if (temp[y][x] >= 4) {
		grid[y][x] = temp[y][x] - 4;
		grid[y - 1][x] += 1;
		grid[y + 1][x] += 1;
		grid[y][x - 1] += 1;
		grid[y][x + 1] += 1;
	  }
	}
  }

  for (int16_t i = 0; i < height; i++) {
	delete[] temp[i];
  }
  delete[] temp;
}

void sandpileModel(uint64_t **&grid, int16_t &width, int16_t &height, int16_t &min_x, int16_t &min_y,
				   int max_iter, int freq, const std::string &output_prefix) {
	int iterations = 0;

#ifdef _WIN32
	system("rmdir /s /q BMP_PHOTOS");
#else
	system("rm -rf BMP_PHOTOS");
#endif

	system("mkdir BMP_PHOTOS");

	while (!isStable(grid, width, height) && (!max_iter || iterations < max_iter)) {
		if (freq > 0 && iterations % freq == 0) {
			std::string filename = "BMP_PHOTOS/" + output_prefix + std::to_string(iterations) + ".bmp";
			saveToBMP(filename, grid, width, height);
		}

		toppleByDivision(grid, width, height, min_x, min_y);
		iterations++;
	}

	saveToBMP("BMP_PHOTOS/" + output_prefix + ".bmp", grid, width, height);
}

Args_from_Terminal TerminalParse(int cn, char *args[]) {
  Args_from_Terminal terminal;
  for (int i = 0; i < cn; i++) {
	char *line = args[i];
	if (strcmp(line, "-i") == 0 || strcmp(line, "--input") == 0) {
	  terminal.input = args[i + 1];
	}
	if (strcmp(line, "-o") == 0 || strcmp(line, "--output") == 0) {
	  terminal.output = args[i + 1];
	}
	if (strcmp(line, "-m") == 0 || strcmp(line, "--max-iter") == 0) {
	  terminal.max_iter = std::stoi(args[i + 1]);
	}
	if (strcmp(line, "-f") == 0 || strcmp(line, "--freq") == 0) {
	  terminal.freq = std::stoi(args[i + 1]);
	}
  }
  return terminal;
}

uint64_t **CreateGrid(const DynamicArray<Cell> &cells, int16_t max_x, int16_t max_y,
					  int16_t min_x, int16_t min_y) {
  int16_t width = max_x - min_x + 1;
  int16_t height = max_y - min_y + 1;

  auto **grid = new uint64_t *[height];
  for (int i = 0; i < height; i++) {
	grid[i] = new uint64_t[width]();
  }

  for (size_t i = 0; i < cells.size(); i++) {
	const Cell &cell = cells[i];
	int16_t x;
	x = cell.x - min_x;
	int16_t y;
	y = cell.y - min_y;
	grid[y][x] = cell.sand;
  }

  return grid;
}

DynamicArray<Cell> ReadCells(const std::string &filename, int16_t &max_x, int16_t &max_y,
							 int16_t &min_x, int16_t &min_y) {
  DynamicArray<Cell> cells;
  std::ifstream file(filename);

  max_x = max_y = std::numeric_limits<int16_t>::min();
  min_x = min_y = std::numeric_limits<int16_t>::max();

  Cell cell{};

  while (file >> cell.x >> cell.y >> cell.sand) {
	if (cell.x > max_x) max_x = cell.x;
	if (cell.y > max_y) max_y = cell.y;
	if (cell.x < min_x) min_x = cell.x;
	if (cell.y < min_y) min_y = cell.y;
	cells.push_back(cell);
  }

  return cells;
}

int ErrorsFind(const Args_from_Terminal &args_for_inspect) {
  if (args_for_inspect.input.empty()) {
	std::cout << "Error: No input file!";
	return 1;
  }
  if (args_for_inspect.output.empty()) {
	std::cout << "Error: No output file!";
	return 1;
  }
  if (args_for_inspect.freq < 0) {
	std::cout << "Error: Frequency must be non-negative!";
	return 1;
  }
  if (args_for_inspect.max_iter < 0) {
	std::cout << "Max iterations must be not negative!";
	return 1;
  }
  return 0;
}

int main(int argc, char *argv[]) {
  Args_from_Terminal terminal = TerminalParse(argc, argv);
  if (ErrorsFind(terminal)) {
	return 1;
  }
  int16_t max_x = 0, max_y = 0, min_x = INT16_MAX, min_y = INT16_MAX;
  DynamicArray<Cell> cells = ReadCells(terminal.input, max_x, max_y, min_x, min_y);
  uint64_t **grid = CreateGrid(cells, max_x, max_y, min_x, min_y);

  int16_t height = max_y - min_y + 1;
  int16_t width = max_x - min_x + 1;

  sandpileModel(grid, width, height, min_x, min_y, terminal.max_iter, terminal.freq, terminal.output);

  for (int i = 0; i < height; i++) {
	delete[] grid[i];
  }
  delete[] grid;
}
