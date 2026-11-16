#include "player.h"

template<typename A, typename B>
bool are_collide(const A& a, const B& b)
{
    return (a.getPosition() - b.getPosition()).length() <= a.getRadius() + b.getRadius();
}
sf::Vector2f rand_pos(float radius, sf::Vector2u bounds)
{
    radius += 5;
    return sf::Vector2f(rand_int(radius, bounds.x - radius), rand_int(radius, bounds.y - radius));
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
    m_text.setPosition({ 300,0 });

    for (short i = 0; i < 10; i++)
    {
        auto& b = m_bubbles.emplace_back(sf::Vector2f{}, m_bubble_texture, sf::degrees(rand_int(0, 360)));
        b.set_position(rand_pos(b.getRadius(), bounds));
    }
    m_text.setString(std::format("There are {} bubbles now", m_bubbles.size()));

    // make them not overlap
    for (auto current = m_bubbles.begin(); current != m_bubbles.end(); ++current)
        for (auto next = std::next(current); next != m_bubbles.end(); ++next)
        {
            if(are_collide(*current,*next))
                 current->set_position(rand_pos(current->getRadius(), m_bounds));
        }
    m_clock.start();
}

// change bubbles directions and speeds, moves them apart
void resolve(Bubble& a, Bubble& b)
{
    sf::Vector2f delta{ a.getPosition() - b.getPosition() };
    float dist{ delta.length() };
    if (dist == 0) return; 

    sf::Vector2f normal{ delta.normalized() };
    sf::Vector2f push{ normal * (((a.getRadius()+b.getRadius())-dist)/2)};
    if( 3.f < push.length())
    {
        a.move_xy(push);
        b.move_xy(-push);
    }

    sf::Vector2f va{ a.m_speed, a.m_angle };
    sf::Vector2f vb{ b.m_speed, b.m_angle };
    if (0 < (va - vb).dot(normal)) return;

    sf::Angle mirror{ normal.angle() * 2 - sf::degrees(180) };
    a.m_angle = mirror - a.m_angle;
    b.m_angle = mirror - b.m_angle;

    std::swap(a.m_speed, b.m_speed);
}
// check and relosve bubble-bullet/window-bullet collision. 
// edit bubble's size/speed/limit after division here (at m_bubbles.emplace_back)
void Space::move_bullets(sf::Time t)
{
    auto& view_bullets = m_bullets.control_bullets();
    auto movement = m_bullets.m_speed * t.asSeconds();


    for (auto bullet = view_bullets.begin(); bullet != view_bullets.end(); )
    {
        bullet->first.move({ movement, bullet->second });
        auto bullet_2x = bullet->first.getRadius() * 2;

        //check and relosve bubble-bullet collision
        for (auto bubble = m_bubbles.begin(); bubble != m_bubbles.end(); )
        {
            if (are_collide(bullet->first,*bubble)) {
                auto temp = std::next(bubble);
                //---------------------------------------
                // devide to a limit
                if (bullet_2x < bubble->getRadius())  // <-------- change limit here
                {
                    const auto pos = bubble->getPosition();
                    constexpr float divisor = 1.5; // <----------- change size/speed here 

                    m_bubbles.emplace_back(pos, m_bubble_texture, bubble->m_angle, (bubble->getScale() / divisor));
                    auto& b1 = m_bubbles.back();
                    b1.m_speed /= divisor;

                    m_bubbles.emplace_back(pos, m_bubble_texture, -bubble->m_angle, (bubble->getScale() / divisor));
                    auto& b2 = m_bubbles.back();
                    b2.m_speed /= divisor;
                    //----------------------
                }
                //---------------------------------------
                m_bubbles.erase(bubble);
                bubble = temp;
            }else ++bubble;
        }

        //check and relosve window-bullet collision
        if (Window_Collision::none!=window_collision(bullet->first.getPosition(), m_bullets.m_radius,m_bounds))
        {
            auto temp = std::next(bullet);
            view_bullets.erase(bullet);
            bullet = temp;
        }
        else ++bullet;
    }
    m_text.setString(std::format("There are {} bubbles now", m_bubbles.size()));
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
            current->move_by(5);
            break;
        case Window_Collision::horizotal:
            current->m_angle = sf::degrees(180) - current->m_angle;
            current->move_by(5);
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
            }
        }
   }
}
//--------------------------------------------
Bubble::Bubble(const sf::Vector2f& pos, const sf::Texture& t, sf::Angle a, float scale)
    :m_sprite{ t }, m_angle{ a }
{
    set_position(pos);
    m_sprite.setOrigin(m_sprite.getLocalBounds().size/2.f);
    m_sprite.scale({ scale,scale });

    m_scale_x = scale;
    m_radius = m_sprite.getLocalBounds().size.x * m_scale_x / 2.f;
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
    :sf::RenderWindow{ sf::VideoMode{ { 1200, 800 } }, "Player",sf::Style::Close ,sf::State::Windowed,sf::ContextSettings{.antiAliasingLevel = 8 } },
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

