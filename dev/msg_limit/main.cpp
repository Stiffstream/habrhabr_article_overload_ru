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

// Вспомогательная функция для вычисления паузы для притормаживания
// рабочей нити.
auto sleeping_pause( unsigned long long v ) {
	return chrono::microseconds( v / 50 );
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
class accurate_resizer final : public agent_t {
public :
	accurate_resizer( context_t ctx, mbox_t overload_mbox )
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
				this_thread::sleep_for( sleeping_pause( px_count ) );
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
				this_thread::sleep_for( sleeping_pause( px_count / 3 ) );
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
				this_thread::sleep_for( sleeping_pause( px_count / 2 ) );
				send< resize_result >( msg.reply_to_, image_handle{
						msg.image_.name_,
						cx,
						cy,
						"empty image" } );
			} );
	}
};

// Вспомогательная функция для создания агентов для рейсайзинга картинок.
// Возвращается mbox, на который следует отсылать запросы для ресайзинга.
mbox_t make_resizers( environment_t & env ) {
	mbox_t resizer_mbox;
	// Все агенты будут иметь собственный контекст исполнения (т.е. каждый агент
	// будет активным объектом), для чего используется приватный диспетчер
	// active_obj.
	env.introduce_coop(
		disp::active_obj::create_private_disp( env )->binder(),
		[&resizer_mbox]( coop_t & coop ) {
			// Создаем агентов в обратном порядке, т.к. нам нужны будут
			// mbox-ы на которые следует перенаправлять "лишние" сообщения.
			auto third = coop.make_agent< empty_image_maker >();
			auto second = coop.make_agent< inaccurate_resizer >( third->so_direct_mbox() );
			auto first = coop.make_agent< accurate_resizer >( second->so_direct_mbox() );

			// Последним создан агент, который будет первым в цепочке агентов
			// для ресайзинга картинок. Его почтовый ящик и будет ящиком для
			// отправки запросов.
			resizer_mbox = first->so_direct_mbox();
		} );

	return resizer_mbox;
}

// Агент, который будет генерировать запросы на ресайзинг картинок
// и обрабатывать результаты ресайзинга.
class requests_initiator final : public agent_t {
	struct next : public signal_t {};
public :
	requests_initiator( context_t ctx, mbox_t resizer_mbox )
		:	agent_t(ctx), resizer_mbox_(move(resizer_mbox))
	{
		so_subscribe_self()
			.event( &requests_initiator::on_next )
			.event( &requests_initiator::on_result );
	}

	virtual void so_evt_start() override {
		send< next >( *this );
	}

private :
	const mbox_t resizer_mbox_;

	chrono::milliseconds last_pause_{ 250 };

	static constexpr unsigned int initial_size = 1024;
	static constexpr unsigned int maximin_size = initial_size * 8;

	unsigned int last_cx_{ initial_size };
	unsigned int last_cy_{ initial_size };

	void on_next( mhood_t< next > ) {
		ostringstream ss;
		ss << "img_" << last_cx_ << "x" << last_cy_ << "-" << last_pause_.count();

		send< resize_request >( resizer_mbox_, so_direct_mbox(),
				image_handle{ ss.str(), last_cx_, last_cy_, string() },
				0.5 );
		send_delayed< next >( *this, last_pause_ );

		update_last_pause();
		last_cx_ = last_cx_ + last_cx_ / 4;
		last_cy_ = last_cy_ + last_cy_ / 4;

		if( last_cx_ > maximin_size ) last_cx_ = initial_size;
		if( last_cy_ > maximin_size ) last_cy_ = initial_size;
	}

	void on_result( const resize_result & msg ) {
		cout << "resize_result: " << msg.image_.name_
				<< " (" << msg.image_.cx_ << "," << msg.image_.cy_ << ") ["
				<< msg.image_.comment_ << "]" << std::endl;
	}

	void update_last_pause() {
		static constexpr chrono::milliseconds minimal{ 20 };
		if( last_pause_ > minimal )
			last_pause_ -= chrono::milliseconds{ 1 };
	}
};

int main() {
	try {
		so_5::launch( []( environment_t & env ) {
			// Создаем кооперацию с агентами для ресайзинга.
			const auto resize_mbox = make_resizers( env );

			// Создаем агента, который будет генерировать запросы на ресайзинг.
			env.register_agent_as_coop( autoname,
				env.make_agent< requests_initiator >( resize_mbox ) );

			// Далее приложение будет работать до тех пор, пока не прервется
			// аварийно из-за превышения лимитов.
		} );
	}
	catch( const exception & x ) {
		cerr << "Exception: " << x.what() << endl;
	}
}

