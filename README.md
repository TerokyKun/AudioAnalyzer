## AudioAnalyzer

# 1. Открыть MSYS2 MINGW64

# 2. Обновить пакеты
pacman -Syu
# После первой команды закрыть окно и открыть MINGW64 снова
pacman -Su

# 3. Установить инструменты
pacman -S mingw-w64-x86_64-cmake
pacman -S mingw-w64-x86_64-ninja
pacman -S mingw-w64-x86_64-toolchain

# 4. Проверить версии установленных инструментов
cmake --version
ninja --version
g++ --version

# 5. Перейти в папку проекта
cd /c/Users/maxkl/Downloads/AudioAnalyzer

# 6. Создать папку для сборки
mkdir build
cd build

# 7. Сконфигурировать проект с помощью CMake и Ninja
cmake -G Ninja ..

# 8. Построить проект
ninja

# 9. Перейти к собранному бинарнику
cd bin

# 10. Создать папку audio рядом с бинарником
mkdir audio

# 11. Поместить аудио файл track1.mp3 в папку audio
# Пример: копирование из исходной папки
cp ../audio/track1.mp3 audio/

# 12. Запустить программу
./AudioVisualizer.exe