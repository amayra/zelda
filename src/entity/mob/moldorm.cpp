#include <iostream>
#include "moldorm.hpp"
#include "action/move.hpp"
#include "ai/rotation_chase.hpp"
#define _USE_MATH_DEFINES
#include "math.h"
#include "../../debug.hpp"
#include "../../game.hpp"
#include "../../audio/music.hpp"
#include "../../graphic/effect/fade.hpp"

SpriteSheet* Moldorm::SPRITESHEET;
SpriteSet* Moldorm::HEAD;
Sprite* Moldorm::MIDDLE1;
Sprite* Moldorm::MIDDLE2;
Sprite* Moldorm::MIDDLE3;
Sprite* Moldorm::TAIL;

void Moldorm::Load() {
    SPRITESHEET = new SpriteSheet("charset/moldorm/body.png", 32, 150, 32, 30);
    TAIL = SPRITESHEET->GetSprites(0, 1)[0];
    MIDDLE1 = SPRITESHEET->GetSprites(1, 1)[0];
    MIDDLE2 = SPRITESHEET->GetSprites(2, 1)[0];
    MIDDLE3 = SPRITESHEET->GetSprites(3, 1)[0];
    HEAD = new SpriteSet(SPRITESHEET->GetSprites(4, 1), 0, 30, vec2f(49, 50));
}

Moldorm::Moldorm(float x, float y, Level* level) :
        super(x, y, 22, 22, new ::Move(this, {HEAD})),
        hitbox_(new MoldormHitbox(this))
{
    set_AI(new RotationChase(this));
    type_ = BOSS;
    is_vulnerable_ = false;
    facing_ = Dir::DOWN;
    rotation = 0;
    speed_ = 100;
    health_ = 20;

    MoldormNode* middle1 = new MoldormNode(x, y, 17, 15, vec2f(-8, -8), 18, MIDDLE3, this, this);
    MoldormNode* middle2 = new MoldormNode(x, y, 17, 15, vec2f(-8, -8), 13, MIDDLE2, this, middle1);
    MoldormNode* middle3 = new MoldormNode(x, y, 12, 12, vec2f(-10, -10), 12, MIDDLE1, this, middle2);
    MoldormNode* tail = new MoldormNode(x, y, 16, 16, vec2f(-8, -8), 12, TAIL, this, middle3);

    nodes_.push_back(tail);
    nodes_.push_back(middle3);
    nodes_.push_back(middle2);
    nodes_.push_back(middle1);

    tail_ = tail;
    level->AddCollidable(hitbox_);
}

void Moldorm::Update(double delta) {
    super::Update(delta);
    facing_ = Dir::DOWN;

    float angle = rotation * (float)M_PI / 180;
    float x = (float)-sin(angle);
    float y = (float)cos(angle);

    if(Move(vec2f(x, 0), 1, delta)) {
        if(!Move(vec2f(0, y), 1, delta)) {
            rotation = 180 - rotation;
        }

        hitbox_->set_position(position_.x - 49, position_.y - 50);
        for(MoldormNode* node : nodes_)
            node->Update(delta);
    } else {
        rotation = 360 - rotation;
    }

    if(health_ < 12) {
        speed_ = 200;
    } else if(health_ < 16) {
        speed_ = 120;
    }
}

void Moldorm::Draw() const {
    for(MoldormNode* node : nodes_)
        node->Render();

    glPushMatrix();
    glTranslatef(position_.x + 11, position_.y + 10, 0);
    glRotatef(rotation, 0, 0, 1.0f);
    CurrentSprite()->Render(vec2f(-16, -15));
    glPopMatrix();

    if(Debug::enabled) {
        for(MoldormNode* node : nodes_)
            node->DrawBox(0, 0, 1);

        hitbox_->DrawBox(0, 0, 1);
    }
}

bool Moldorm::CanCollideWith(RectangleShape* rectangle) const {
    return rectangle != hitbox_ && (!rectangle->IsEntity() || ((Entity*)rectangle)->type() == PLAYER);
}

bool Moldorm::CollidesWith(RectangleShape const * rectangle) const {
    if(rectangle->IsEntity()) {
        for(MoldormNode* node : nodes_) {
            if(node->CollidesWith(rectangle))
                return true;
        }
    }

    return super::CollidesWith(rectangle);
}

Moldorm::MoldormNode::MoldormNode(float x, float y, float width, float height, const vec2f& offset, float max_distance,
        Sprite* sprite, Moldorm* head, RectangleShape* parent) :
        super(x, y, width, height),
        offset_(offset),
        max_distance_(max_distance),
        sprite_(sprite),
        head_(head),
        parent_(parent)
{}

void Moldorm::MoldormNode::Update(double delta) {
    if(parent_->Distance(this) > max_distance_) {
        vec2f dir = parent_->center() - center();
        dir.normalize();
        dir = dir * head_->speed() * delta;

        set_position(position_.x + dir.x, position_.y + dir.y);
    }
}

void Moldorm::MoldormNode::Draw() const {
    sprite_->Render(position_ + offset_);
}

Moldorm::MoldormHitbox::MoldormHitbox(Moldorm* moldorm) :
        super(moldorm->position().x - 49, moldorm->position().y - 50, 130, 130),
        moldorm_(moldorm)
{
    type_ = BOSS;
}

bool Moldorm::MoldormHitbox::CanCollideWith(RectangleShape* rectangle) const {
    return moldorm_ != rectangle && moldorm_->CanCollideWith(rectangle);
}

bool Moldorm::HandleCollisionWith(Mob* mob) {
    mob->Damage(this, 2);
    return true;
}

bool Moldorm::MoldormHitbox::HandleCollisionWith(Mob* mob) {
    return moldorm_->HandleCollisionWith(mob);
}

void Moldorm::MoldormHitbox::Damage(Entity* from, int amount) {
    moldorm_->Hit();
}

Sprite* Moldorm::MoldormHitbox::CurrentSprite(vec2f& position) const {
    position = moldorm_->tail()->position() - position_;
    return TAIL;
}

Moldorm::MoldormNode* Moldorm::tail() const {
    return tail_;
}

Sprite* Moldorm::MoldormHitbox::CurrentSprite() const {
    return TAIL;
}

bool Moldorm::MoldormHitbox::CollidesWith(RectangleShape const* rectangle) const {
    return moldorm_->CollidesWith(rectangle);
}

Moldorm::~Moldorm() {
    delete hitbox_;

    for(MoldormNode* node : nodes_)
        delete node;
}

void Moldorm::Dead() {
    level_->RemoveCollidable(hitbox_);

    Music::ClearQueue();
    Music::FadeOut(2);

    level_->ChangeEffect(new Fade(Fade::FADE_OUT, 2, []{
        Game::INSTANCE.Win();
    }));
}

void Moldorm::Hit() {
    health_ -= 1;
}

void Moldorm::Rotate() {
    rotation += 2;
}

vec2f Moldorm::direction() const {
    float angle = rotation * (float)M_PI / 180;
    return vec2f((float)-sin(angle), (float)cos(angle));
}
