# Что это?

Это пример для статьи "SObjectizer: проблема перегрузки агентов и средства борьбы с ней" на Habrhabr.ru. Пример можно взять, скомпилировать и поиграться с исходниками.

# Как взять и попробовать?

Для компиляции примера потребуется Ruby, RubyGems и Rake. Обычно все эти инструменты идут в одном пакете. Но может потребоваться устанавливать их по отдельности. Например:
~~~~~
::bash
sudo apt install ruby
sudo apt install rake
~~~~~
После установки Ruby (+RubyGems+Rake) нужно установить Mxx_ru:
~~~~~
::bash
gem install Mxx_ru
~~~~~
Или, если gem требует прав администратора:
~~~~~
::bash
sudo gem install Mxx_ru
~~~~~
После этого можно забрать исходный код примера с BitBucket-а и компилировать:
~~~~~
::bash
# Забираем исходники Mercurial-ом
hg clone https://bitbucket.org/sobjectizerteam/habrhabr_article_overload
cd habrhabr_article_overload
# Забираем все необходимые зависимости.
mxxruexternals
# Компилируем.
cd dev
ruby build.rb
~~~~~
Либо же, без Mercurial-а:
~~~~~
::bash
# Забираем и распаковываем исходники
wget https://bitbucket.org/sobjectizerteam/habrhabr_article_overload/get/tip.tar.bz2
tar xaf tip.tar.bz2
cd <каталог, который получился после распаковки>
# Забираем все необходимые зависимости.
mxxruexternals
# Компилируем.
cd dev
ruby build.rb
~~~~~
В результате компиляции в target/release должны оказаться libso-5.5.17.1.so и приложение msg_limit_app.

Аналогичные действия нужно предпринимать и под Windows. Под Windows в каталоге target/release окажутся файлы so-5.5.17.1.dll и приложение msg_limit_app.
