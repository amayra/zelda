#include "dialog.hpp"
#include "../../game.hpp"
#include "../font.hpp"

Dialog::Message::Message(const std::string& line1, const std::string& line2, float speed) {
    this->line1 = line1;
    this->line2 = line2;
    this->speed = speed;
    this->length = (int)(line1.size() + line2.size());
}

Dialog::Dialog(std::vector<Message*> const messages) :
        messages_(messages),
        elapsed_time_(0),
        current_message_(0)
{}

void Dialog::Tick(double delta) {
    elapsed_time_ += delta;

    Message* current = messages_[current_message_];
    if(elapsed_time_ * current->speed >= current->length and Game::INSTANCE.ConsumeKey(Game::ACTION_KEY)) {
        current_message_++;
        elapsed_time_ = 0;
    }
}

void Dialog::Render() const {
    drawable_->Draw();

    if(current_message_ < messages_.size()) {
        vec2f line1_position = vec2f(Game::WIDTH - Game::MIN_WIDTH, Game::HEIGHT - 74);
        vec2f line2_position = vec2f(Game::WIDTH - Game::MIN_WIDTH, Game::HEIGHT - 37);

        Message* current = messages_[current_message_];
        unsigned int until = (unsigned int)(elapsed_time_ * current->speed);

        if(until >= current->length) {
            Font::Render(line1_position.x, line1_position.y, current->line1.c_str());
            Font::Render(line2_position.x, line2_position.y, current->line2.c_str());
        } else if(until >= current->line1.size()) {
            Font::Render(line1_position.x, line1_position.y, current->line1.c_str());
            Font::Render(line2_position.x, line2_position.y,
                    current->line2.substr(0, until - current->line1.size()).c_str());
        } else {
            Font::Render(line1_position.x, line1_position.y, current->line1.substr(0, until).c_str());
        }
    }
}

bool Dialog::IsFinished() const {
    return current_message_ >= messages_.size();
}
