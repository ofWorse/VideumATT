# VideumATT - Video Annotation Train Tool 

Приложение для ручной аннотации изображений и запуска обучения детектора объектов.

## Что реализовано

1. GUI полностью на C++ (без `.ui`): `class MainWindow : public QWidget`.
2. Выбор папки датасета и загрузка списка изображений.
3. Добавление классов объектов (имен для аннотации).
4. Ручная разметка `bounding box` по изображениям.
5. Разные цвета для bbox разных классов.
6. Сохранение состояния проекта в `.annotate/project.json`.
7. Кнопка `Start train`:
   - экспортирует датасет в YOLO-формат (`.annotate/export_yolo`);
   - запускает обучение через Python-скрипт `scripts/train_yolo.py`;
   - экспортирует обученную модель в ONNX.
8. Подготовлен C++ модуль интеграции ONNX-инференса через OpenCV DNN (`integration/`).

## Структура

- `Annotate.pro` - qmake проект.
- `src/` - Qt/C++ приложение.
- `scripts/train_yolo.py` - обучение YOLO (Ultralytics).
- `integration/` - пример подключения ONNX в сторонний C++ проект.

## Сборка (Windows, Qt 5.15.2)

```powershell
qmake Annotate.pro
nmake
```

Или через `jom`:

```powershell
qmake Annotate.pro
jom
```

## Использование

1. Запустить приложение.
2. Нажать `Open Dataset Folder`.
3. Добавить классы в блоке `Object classes`.
4. Выбрать активный класс (`Active class`).
5. Рисовать bbox мышью на канве.
6. Переключаться между изображениями (`Prev/Next`).
7. Нажать `Save Project State` для сохранения.
8. Нажать `Start train` для запуска обучения.

## Зависимости для обучения

Приложение вызывает Python:

```powershell
pip install ultralytics
```

Скрипт:

- использует базовую модель `yolov8n.pt`;
- сохраняет веса в `.annotate/export_yolo/runs/annotate_train/weights/best.pt`;
- экспортирует ONNX.

## Интеграция в сторонние C++ проекты

Используйте файлы:

- `integration/detector_onnx_opencv.h`
- `integration/detector_onnx_opencv.cpp`

Пример запуска на видеопотоке:

- `integration/inference_example.cpp`

Нужно подключить OpenCV с модулем `dnn`.

