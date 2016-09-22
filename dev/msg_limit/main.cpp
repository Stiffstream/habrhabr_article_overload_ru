/*
 * Пример использования message limits для защиты агентов от перегрузки.
 */

#include <so_5/all.hpp>

using namespace std;
using namespace so_5;

// Вспомогательные функции дабы не писать static_cast.
template< typename T >
auto to_ull( T v ) {
	return static_cast< unsigned long long >(v);
}

template< typename T >
auto to_ui( T v ) {
	return static_cast< unsigned int >(v);
}

// Вместо реальных картинок используем данный тип для имитации.
struct image_handle {
	string name_; // Какое-то название.
	unsigned int cx_; // Размер картинки по X.
	unsigned int cy_; // Размер картинки по Y.
	// Специальное поле, которое позволит понять, кто, что и как делал
	// с картинкой.
	string comment_;
};

// Запрос, который отсылается для того, чтобы отресайзить картинку.
struct resize_request : public message_t {
	// Кому должен уйти результат ресайзинга.
	mbox_t reply_to_;
	// Картинка для ресайза.
	image_handle image_;
	// Во сколько раз нужно изменить размер картинки.
	float scale_factor_;	

	resize_request( mbox_t reply_to, image_handle image, float scale_factor )
		:	reply_to_(move(reply_to)), image_(move(image)), scale_factor_(scale_factor)
	{}
};

// Ответ с результатом ресайза.
struct resize_result : public message_t {
	// Результирующее изображение.
	image_handle image_;

	resize_result( image_handle image )
		:	image_(move(image))
	{}
};

// Тип агента, который выполняет нормальную обработку картинок.
class normal_resizer final : public agent_t {
public :
	normal_resizer( context_t ctx, mbox_t overload_mbox )
		// Лимиты настраиваются при создании агента и не могут меняться
		// впоследствии. Поэтому конструируем лимиты сообщений как уточнение
		// контекста, в котором агенту предстоит работать дальше.
		:	agent_t( ctx
				// Ограничиваем количество ждущих соообщений resize_request
				// с автоматическим редиректом лишних экземпляров другому
				// обработчику.
				+ limit_then_redirect< resize_request >(
					// Разрешаем хранить всего 10 запросов...
					10,
					// Остальное пойдет на этот mbox.
					[overload_mbox]() { return overload_mbox; } ) )
	{
		// Простая имитация обработки изобращения.
		so_subscribe_self().event( []( const resize_request & msg ) {
				// Приостанавливаем выполнение текущей нити на время,
				// пропорциональное размеру картинки.
				const auto px_count = to_ull( msg.image_.cx_ ) * msg.image_.cy_;
				this_thread::sleep_for( chrono::nanoseconds{px_count} );
				// Имитируем нормальный ответ.
				send< resize_result >( msg.reply_to_, image_handle{
						msg.image_.name_,
						to_ui( msg.image_.cx_ * msg.scale_factor_ ),
						to_ui( msg.image_.cy_ * msg.scale_factor_ ),
						"accurate resizing" } );
			} );
	}
};

// Тип агента, который выполняет грубую обработку картинок.
class inaccurate_resizer final : public agent_t {
public :
	inaccurate_resizer( context_t ctx, mbox_t overload_mbox )
		:	agent_t( ctx
				+ limit_then_redirect< resize_request >(
					// Разрешаем хранить всего 20 запросов...
					20,
					// Остальное пойдет на этот mbox.
					[overload_mbox]() { return overload_mbox; } ) )
	{
		// Простая имитация обработки изобращения.
		so_subscribe_self().event( []( const resize_request & msg ) {
				const auto px_count = to_ull( msg.image_.cx_ ) * msg.image_.cy_;
				this_thread::sleep_for( chrono::nanoseconds{px_count / 3} );
				send< resize_result >( msg.reply_to_, image_handle{
						msg.image_.name_,
						to_ui( msg.image_.cx_ * msg.scale_factor_ ),
						to_ui( msg.image_.cy_ * msg.scale_factor_ ),
						"inaccurate resizing" } );
			} );
	}
};

// Тип агента, который не выполняет никакой обработки картинки,
// а вместо этого возвращает пустую картинку заданного размера.
class empty_image_maker final : public agent_t {
public :
	empty_image_maker( context_t ctx )
		:	agent_t( ctx
				// Ограничиваем количество запросов 50-тью штуками.
				// Если их все-таки оказывается больше, значит что-то идет
				// совсем не так, как задумывалось и лучше прервать работу
				// приложения.
				+ limit_then_abort< resize_request >( 50 ) )
	{
		so_subscribe_self().event( []( const resize_request & msg ) {
				const auto cx = to_ui( msg.image_.cx_ * msg.scale_factor_ );
				const auto cy = to_ui( msg.image_.cy_ * msg.scale_factor_ );
				const auto px_count = to_ull( cx ) * cy;
				this_thread::sleep_for( chrono::nanoseconds{px_count / 2} );
				send< resize_result >( msg.reply_to_, image_handle{
						msg.image_.name_,
						cx,
						cy,
						"empty image" } );
			} );
	}
};

int main() {
}

