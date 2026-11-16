#include "player.h"

template<typename A, typename B>
bool are_collide(const A& a, const B& b)
{
    return (a.getPosition() - b.getPosition()).length() <= a.getRadius() + b.getRadius();
}
sf::Vector2f rand_pos(sf::Vector2u bounds)
{
    return sf::Vector2f(rand_int(50, bounds.x - 50), rand_int(50, bounds.y - 50));
}
void push_away(Bubble& a, Bubble& b)
{
    while (are_collide(a, b)) 
    {
        a.move(sf::milliseconds(20));
        b.move(sf::milliseconds(20));
    }
}
enum class Window_Collision {none,vertical,horizotal};
Window_Collision window_collision(sf::Vector2f pos,float radius, sf::Vector2u bounds)
{
    if ((pos.x - radius) < 0 || bounds.x < (pos.x + radius)) return Window_Collision::horizotal;
    if ((pos.y - radius < 0) || (bounds.y < pos.y + radius) ) return Window_Collision::vertical;
    return Window_Collision::none;
}
//--------------------------------------------
void Space::tick_iteration() 
{
    auto time_elapced = m_clock.restart();

    move_bullets(time_elapced);
    m_player(time_elapced);
    move_bubbles(time_elapced);
}
Space::Space(Bullets&b,  Player& p, sf::Vector2u bounds)
    :m_bullets{b},m_player{p},m_bounds{ bounds },m_text{font}
{
    m_text.setFillColor(sf::Color::White);
    m_text.setPosition({ 250,0 });

    for (short i = 0; i < 10; i++)
    {
        m_bubbles.emplace_back(rand_pos(bounds), m_bubble_texture);
    }
    m_text.setString(std::format("There are {} bubbles now", m_bubbles.size()));

    // make them not overlap
    for (auto current = m_bubbles.begin(); current != m_bubbles.end(); ++current)
        for (auto next = std::next(current); next != m_bubbles.end(); ++next)
            push_away(*current, *next);

    m_clock.start();
}

// check and relosve bubble-bullet/window-bullet collision. 
// edit bubble's size/speed/limit after division here (at m_bubbles.emplace_back)
void Space::move_bullets(sf::Time t)
{
    auto& view_bullets = m_bullets.control_bullets();
    auto movement = m_bullets.m_speed * t.asSeconds();
    for (auto current = view_bullets.begin(); current != view_bullets.end(); )
    {
        current->first.move({ movement, current->second });

        //check and relosve bubble-bullet collision
        for (auto bubble = m_bubbles.begin(); bubble != m_bubbles.end(); )
        {
            if (are_collide(current->first,*bubble)) {
                auto temp = std::next(bubble);
                //---------------------------------------
                // devide to a limit
                // change limit here ------------->
                if (0.03 < bubble->getScale().x)
                {
                    auto pos = bubble->getPosition();
                    //change size/speed here ------------->
                    float divisor = 1.5;
                    
                    m_bubbles.emplace_back(pos, m_bubble_texture, (bubble->getScale().x / divisor));
                    m_bubbles.back().m_speed /= divisor;
                    m_bubbles.emplace_back(pos, m_bubble_texture, (bubble->getScale().x / divisor));
                    m_bubbles.back().m_speed /= divisor;

                    // make them not overlap
                    push_away(*std::prev(m_bubbles.end(), 1), *std::prev(m_bubbles.end(), 2));
                }
                //---------------------------------------
                m_bubbles.erase(bubble);
                bubble = temp;
            }else ++bubble;
        }

        //check and relosve window-bullet collision
        if (Window_Collision::none!=window_collision(current->first.getPosition(), m_bullets.m_radius,m_bounds))
        {
            auto temp = std::next(current);
            view_bullets.erase(current);
            current = temp;
        }
        else ++current;
    }
    m_text.setString(std::format("There are {} bubbles now", m_bubbles.size()));
}
// change bubbles directions and speeds
void resolve(Bubble& a, Bubble& b)
{
    const auto dist = a.getPosition() - b.getPosition();
    const sf::Angle mirror{ dist.angle() * 2 + sf::degrees(180) };
    a.m_angle = mirror - a.m_angle;
    b.m_angle = mirror - b.m_angle;
    std::swap(a.m_speed, b.m_speed);
}

//move, check and relosve bubble-bubble/bubble-Window collisions
void Space::move_bubbles(sf::Time t)
{
    for (auto current = m_bubbles.begin(); current != m_bubbles.end(); ++current )
    {
        current->move(t);

        switch (window_collision(current->getPosition(), current->getRadius(), m_bounds))
        {
        case Window_Collision::none:
            break;
        case Window_Collision::vertical:
            current->m_angle = -current->m_angle;
            break;
        case Window_Collision::horizotal:
            current->m_angle = sf::degrees(180) - current->m_angle;
            break;
        }
    }
   //check and relosve bubble-bubble collision
   for (auto current = m_bubbles.begin(); current != m_bubbles.end(); ++current)
   {
        for (auto next = std::next(current); next != m_bubbles.end(); ++next)
        {
            if (are_collide(*current, *next))
            {
                resolve(*current, *next);
                push_away(*current, *next);
            }
        }
   }
}
//--------------------------------------------
Bubble::Bubble(const sf::Vector2f& pos, const sf::Texture& t,float scale)
    :m_sprite{ t }
{
    set_position(pos);
    m_sprite.setOrigin(m_sprite.getLocalBounds().size/2.f);
    m_sprite.scale({ scale,scale });
}
float Bubble::getRadius() const
{
    sf::FloatRect local = m_sprite.getLocalBounds();
    sf::Vector2f scale = m_sprite.getScale();
    float radius = local.size.x * scale.x / 2.f;
    return radius;
}
//--------------------------------------------
void Bullets::shoot_direction(sf::Vector2f from, sf::Angle angle)
{
    m_bullets.emplace_back(m_radius, angle);
    m_bullets.back().first.setOrigin(m_origin);
    m_bullets.back().first.setPosition(from);
    m_bullets.back().first.setFillColor(sf::Color::Magenta);
}
//--------------------------------------------
Input_Handler::Input_Handler(Player& snake,Bullets& b)
    :m_player{ snake }, m_bullets{b}
{
}
void Input_Handler::operator()(sf::Event::MouseButtonPressed&m)
{
    m_bullets.shoot_direction(m_player.getPosition(),(static_cast<sf::Vector2f>(m.position)-m_player.getPosition()).angle());
}
void Input_Handler::operator()(sf::Event::KeyPressed& k)
{
    switch (k.scancode)
    {
    case sf::Keyboard::Scancode::LShift:
        m_player.speed_up();
        break;
    case sf::Keyboard::Scancode::A:
        m_player.add_movement(sf::degrees(180));
        break;
    case sf::Keyboard::Scancode::S:
        m_player.add_movement(sf::degrees(90));
        break;
    case sf::Keyboard::Scancode::D:
        m_player.add_movement(sf::degrees(0));
        break;
    case sf::Keyboard::Scancode::W:
        m_player.add_movement(sf::degrees(270));
        break;
    }
}
void Input_Handler::operator()(sf::Event::KeyReleased& k)
{
    switch (k.scancode)
    {
    case sf::Keyboard::Scancode::LShift:
        m_player.slow_down();
        break;
    case sf::Keyboard::Scancode::A:
        m_player.remove_movement(sf::degrees(180));
        break;
    case sf::Keyboard::Scancode::S:
        m_player.remove_movement(sf::degrees(90));
        break;
    case sf::Keyboard::Scancode::D:
        m_player.remove_movement(sf::degrees(0));
        break;
    case sf::Keyboard::Scancode::W:
        m_player.remove_movement(sf::degrees(270));
        break;
    }
}
//--------------------------------------------
Player::Player()
{
    m_body.setRadius(m_radius);
    m_body.setPosition({ 300, 300 });
    m_body.setFillColor(sf::Color::White);
    m_body.setOrigin(m_origin);
}
void Player::operator()(sf::Time t)
{
    auto movement = m_speed* t.asSeconds();
    for(const sf::Angle& a: m_movement_angles)
        m_body.move({ movement, a });
}
void Player::remove_movement(sf::Angle angle)
{
    m_movement_angles.erase(std::ranges::find(m_movement_angles,angle));
}
void Player::add_movement(sf::Angle angle)
{
    m_movement_angles.emplace_back(angle);
}
//--------------------------------------------
Timer::Timer() :
    text{ font,"timer-here" }
{
    text.setPosition({ 0, 0 });
    text.setFillColor(sf::Color::Yellow);
}
void Timer::set_text_formatted(const std::string& message)
{
    text.setString(std::format("{:.3}s {}", clock.getElapsedTime().asSeconds(), message));
}
//--------------------------------------------
Window::Window()
    :sf::RenderWindow{ sf::VideoMode{ { 800, 800 } }, "Player",sf::Style::Close ,sf::State::Windowed,sf::ContextSettings{.antiAliasingLevel = 8 } },
    m_move_handler{m_player,m_bullets},
    m_space{ m_bullets,m_player,getSize()}
{
    setKeyRepeatEnabled(false);
}
void Window::show()
{
    short apples_count{ 0 };
    bool not_over{ true };
    m_timer.start();
    while (isOpen())
    {
        handleEvents([this](const sf::Event::Closed) { close(); }, m_move_handler);
       
        m_space.tick_iteration();

        if(not_over) m_timer.set_text_formatted("");
        
        clear(sf::Color{ 18, 20, 32 });

        draw(m_timer);
        draw(m_bullets);
        draw(m_space);
        draw(m_player);

        display();
    }
}
//--------------------------------------------

int main()
try
{
   Window w;
   w.show();
}
catch(const std::exception& e)
{
    std::ofstream ofs{"error_log.txt",std::ios::app};
    ofs << "Error: " << e.what() <<'.' << std::endl;
    std::println("Error: {}.", e.what());
    std::cin.get();
}
catch(...)
{
    std::ofstream ofs{ "error_log.txt",std::ios::app };
    ofs << "Some error." << std::endl;
    std::println("Some error.");
    std::cin.get();
}

