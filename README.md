# Что это?

Это пример для статьи "SObjectizer: проблема перегрузки агентов и средства борьбы с ней" на habr.com. Пример можно взять, скомпилировать и поиграться с исходниками.

# Как взять и попробовать?

Для компиляции примера потребуется Ruby, RubyGems и Rake. Обычно все эти инструменты идут в одном пакете. Но может потребоваться устанавливать их по отдельности. Например:

```sh
sudo apt install ruby
sudo apt install rake
```

После установки Ruby (+RubyGems+Rake) нужно установить Mxx_ru:

```sh
gem install Mxx_ru
```

Или, если gem требует прав администратора:

```sh
sudo gem install Mxx_ru
```

После этого можно забрать исходный код примера с GitHub-а и компилировать:

```sh
# Забираем исходники Git-ом
hg clone https://github.com/stiffstream/habrhabr_article_overload_ru
cd habrhabr_article_overload_ru
# Забираем все необходимые зависимости.
mxxruexternals
# Компилируем.
cd dev
ruby build.rb
```

В результате компиляции в target/release должны оказаться libso-5.5.24.4.so и приложение msg_limit_app.

Аналогичные действия нужно предпринимать и под Windows. Под Windows в каталоге target/release окажутся файлы so-5.5.24.4.dll и приложение msg_limit_app.
