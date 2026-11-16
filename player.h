#pragma once
/*
* Move with WASD, sprint with Shift, shoot with mouse click.
* Bubbles multiply on pop.
*/
#include <SFML/Graphics.hpp>
#include <format>
#include <print>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <list>

sf::Font font{ "OpenSans.ttf" };

inline int rand_int(int min, int max)
{
    static std::default_random_engine ran{ unsigned(time(0)) };
    return std::uniform_int_distribution<>{min, max}(ran);
}
//--------------------------------------------------------------------------------------------
// can  move
class Player : public sf::Drawable
{
public:
    Player();

    sf::Vector2f getPosition() const {  return m_body.getPosition();  }
    const auto& getRadius() const { return m_radius; }

    void speed_up() { m_speed *= 3; }; // Shift pressed
    void slow_down() { m_speed /= 3; }; // Shift released
    inline void operator()(sf::Time); // moves on every stored engles
    
    inline void remove_movement(sf::Angle angle); // KeyReleased
    inline void add_movement(sf::Angle angle);// KeyPressed 
private:
    std::list<sf::Angle> m_movement_angles; // 1 angle per pressed button
    float m_radius{ 16 };
    float m_speed{ 100 };
    sf::CircleShape m_body;
    sf::Vector2f m_origin{ m_radius,m_radius};

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override
    {
        target.draw(m_body, states);
    }
};

//--------------------------------------------------------------------------------------------
//colliding bubble multiplyed when poped
class Bubble : public sf::Drawable
{
public:
    Bubble(const sf::Vector2f& pos, const sf::Texture&, sf::Angle,float scale = 0.15);
  
    const float& getRadius() const { return m_radius; }
    const float& getScale() const { return m_scale_x; }

    sf::Vector2f getPosition() const { return m_sprite.getPosition(); }

    void set_position(const sf::Vector2f& p) { m_sprite.setPosition(p); }
    void move(sf::Time t) { m_sprite.move({ t.asSeconds() * m_speed,m_angle }); }
    void move_xy(const sf::Vector2f& xy) { m_sprite.move(xy); }
    void move_by(float distance) { m_sprite.move({ distance,m_angle }); }
    float m_speed{ 120 };
    sf::Angle m_angle;
private:
    float m_radius;
    float m_scale_x;
    sf::Sprite m_sprite;
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override
    {
        target.draw(m_sprite, states);
    }
};
//--------------------------------------------------------------------------------------------
//Bullets that were shot, it can be multiple oon the screen, move faster than player
class Bullets : public sf::Drawable
{
public:
    inline void shoot_direction(sf::Vector2f from, sf::Angle angle); // lounch a bullet
    auto& control_bullets() { return m_bullets; } // for controolling its lifetime
    const float m_radius{ 3 };
    const float m_speed{ 100 * 6 };
private:
    std::list<std::pair<sf::CircleShape, sf::Angle>> m_bullets;
    sf::Vector2f m_origin{ m_radius,m_radius };

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override
    {
        for (const auto& c : m_bullets)
            target.draw(c.first, states);
    }
};
//--------------------------------------------------------------------------------------------
// input for the Player. adds/removes movements, and (only)lounches bullets
class Input_Handler
{
public:
    Input_Handler(Player&, Bullets&);
    inline void operator()(sf::Event::MouseButtonPressed&);
    inline void operator()(sf::Event::KeyPressed& k);
    inline void operator()(sf::Event::KeyReleased& k);
private:
    Player& m_player;
    Bullets& m_bullets;
};
//--------------------------------------------------------------------------------------------
// Space knows about every object, time and responsible for movement, and lifetime of objects
class Space :public sf::Drawable
{
public:
    Space(Bullets&, Player&, sf::Vector2u);
    inline void tick_iteration(); // call each frame
    std::list<Bubble> m_bubbles;
private:
    inline void move_bullets(sf::Time t);
    inline void move_bubbles(sf::Time t);
    sf::Texture m_bubble_texture{ "BubbleSimple.png" };
    sf::Clock m_clock;
    sf::Vector2u m_bounds;
    Bullets& m_bullets;
    Player& m_player;
    sf::Text m_text;
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override
    {
        target.draw(m_text, states);
        for (const auto& c : m_bubbles)
            target.draw(c, states);
    }
};

//--------------------------------------------------------------------------------------------
// just a textual timer for viewiwing time
class Timer : public sf::Drawable
{
public:
    Timer();
    void start() { clock.start(); }
    void set_text_formatted(const std::string& message);
    void stop() { clock.stop(); }
private:
    sf::Clock clock;
    sf::Text text;
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override
    {
        target.draw(text, states);
    }
};
//--------------------------------------------------------------------------------------------
class Window :public sf::RenderWindow
{
public:
    Window();
    void show();
private:
    Bullets m_bullets;
    Player m_player;
    Input_Handler m_move_handler;
    Timer m_timer;
    Space m_space;
};
//--------------------------------------------------------------------------------------------
