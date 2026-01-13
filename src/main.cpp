#include <cmath>

#include "raylib.h"

struct Ship {
  Vector2 pos;
  Vector2 vel;
  float angle;
  float angularSpeed;
  float thrustPower;
  float damping;
};

struct Bullet {
  bool active;
  Vector2 pos;
  Vector2 vel;
  float life;
};

struct Asteroid {
  Vector2 pos;
  Vector2 vel;
  float radius;
};

static Vector2 RotatePoint(Vector2 p, float angle) {
  float c = cosf(angle);
  float s = sinf(angle);
  return {p.x * c - p.y * s, p.x * s + p.y * c};
}

static Vector2 WrapPosition(Vector2 pos, float w, float h) {
  if (pos.x < 0.0f) pos.x += w;
  if (pos.x > w) pos.x -= w;
  if (pos.y < 0.0f) pos.y += h;
  if (pos.y > h) pos.y -= h;
  return pos;
}

static void DrawPolylineClosed(const Vector2* points, int count, Vector2 offset,
                               float angle, float scale, Color color) {
  for (int i = 0; i < count; ++i) {
    Vector2 a = RotatePoint({points[i].x * scale, points[i].y * scale}, angle);
    Vector2 b = RotatePoint(
        {points[(i + 1) % count].x * scale, points[(i + 1) % count].y * scale},
        angle);
    a.x += offset.x;
    a.y += offset.y;
    b.x += offset.x;
    b.y += offset.y;
    DrawLineV(a, b, color);
  }
}

static void DrawPolylineOpen(const Vector2* points, int count, Vector2 offset,
                             float angle, float scale, Color color) {
  for (int i = 0; i < count - 1; ++i) {
    Vector2 a = RotatePoint({points[i].x * scale, points[i].y * scale}, angle);
    Vector2 b =
        RotatePoint({points[i + 1].x * scale, points[i + 1].y * scale}, angle);
    a.x += offset.x;
    a.y += offset.y;
    b.x += offset.x;
    b.y += offset.y;
    DrawLineV(a, b, color);
  }
}

int main() {
  const int screenWidth = 960;
  const int screenHeight = 540;

  InitWindow(screenWidth, screenHeight, "Asteroids Ship");
  SetTargetFPS(60);

  Ship ship;
  ship.pos = {(float)screenWidth * 0.5f, (float)screenHeight * 0.5f};
  ship.vel = {0.0f, 0.0f};
  ship.angle = -PI / 2.0f;
  ship.angularSpeed = 3.5f;
  ship.thrustPower = 400.0f;
  ship.damping = 1.5f;
  const float shipScale = 1.5f;
  const int MAX_BULLETS = 16;
  const float bulletSpeed = 420.0f;
  const float bulletLife = 1.2f;
  const float bulletLength = 8.0f;
  const float fireInterval = 0.15f;
  float fireCooldown = 0.0f;

  Bullet bullets[MAX_BULLETS];
  for (int i = 0; i < MAX_BULLETS; ++i) {
    bullets[i].active = false;
    bullets[i].pos = {0.0f, 0.0f};
    bullets[i].vel = {0.0f, 0.0f};
    bullets[i].life = 0.0f;
  }

  const Vector2 shipShape[] = {
      {12.0f, 0.0f},
      {-8.0f, -6.0f},
      {-4.0f, 0.0f},
      {-8.0f, 6.0f},
  };

  const Vector2 flameShape[] = {
      {-8.0f, -6.0f}, {-4.0f, 0.0f},  {-8.0f, 6.0f},
      {-16.0f, 0.0f}, {-8.0f, -6.0f},
  };

  while (!WindowShouldClose()) {
    float dt = GetFrameTime();

    float turn = 0.0f;
    if (IsKeyDown(KEY_LEFT)) turn -= 1.0f;
    if (IsKeyDown(KEY_RIGHT)) turn += 1.0f;
    ship.angle += turn * ship.angularSpeed * dt;

    Vector2 forward = {cosf(ship.angle), sinf(ship.angle)};
    bool thrusting = IsKeyDown(KEY_UP);
    if (thrusting) {
      ship.vel.x += forward.x * ship.thrustPower * dt;
      ship.vel.y += forward.y * ship.thrustPower * dt;
    }

    if (fireCooldown > 0.0f) fireCooldown -= dt;
    if (IsKeyDown(KEY_SPACE) && fireCooldown <= 0.0f) {
      for (int i = 0; i < MAX_BULLETS; ++i) {
        if (!bullets[i].active) {
          Vector2 noseLocal = {shipShape[0].x * shipScale,
                               shipShape[0].y * shipScale};
          Vector2 noseOffset = RotatePoint(noseLocal, ship.angle);
          bullets[i].active = true;
          bullets[i].pos = {ship.pos.x + noseOffset.x,
                            ship.pos.y + noseOffset.y};
          bullets[i].vel = {ship.vel.x + forward.x * bulletSpeed,
                            ship.vel.y + forward.y * bulletSpeed};
          bullets[i].life = bulletLife;
          fireCooldown = fireInterval;
          break;
        }
      }
    }

    ship.vel.x -= ship.vel.x * ship.damping * dt;
    ship.vel.y -= ship.vel.y * ship.damping * dt;

    ship.pos.x += ship.vel.x * dt;
    ship.pos.y += ship.vel.y * dt;
    ship.pos = WrapPosition(ship.pos, (float)screenWidth, (float)screenHeight);

    for (int i = 0; i < MAX_BULLETS; ++i) {
      if (!bullets[i].active) continue;
      bullets[i].pos.x += bullets[i].vel.x * dt;
      bullets[i].pos.y += bullets[i].vel.y * dt;
      bullets[i].pos =
          WrapPosition(bullets[i].pos, (float)screenWidth, (float)screenHeight);
      bullets[i].life -= dt;
      if (bullets[i].life <= 0.0f) bullets[i].active = false;
    }

    BeginDrawing();
    ClearBackground(BLACK);

    for (int i = 0; i < MAX_BULLETS; ++i) {
      if (!bullets[i].active) continue;
      float speed = sqrtf(bullets[i].vel.x * bullets[i].vel.x +
                          bullets[i].vel.y * bullets[i].vel.y);
      Vector2 dir = {1.0f, 0.0f};
      if (speed > 0.001f) {
        dir.x = bullets[i].vel.x / speed;
        dir.y = bullets[i].vel.y / speed;
      }
      Vector2 tail = {bullets[i].pos.x - dir.x * bulletLength * 0.5f,
                      bullets[i].pos.y - dir.y * bulletLength * 0.5f};
      Vector2 head = {bullets[i].pos.x + dir.x * bulletLength * 0.5f,
                      bullets[i].pos.y + dir.y * bulletLength * 0.5f};
      DrawLineV(tail, head, RAYWHITE);
    }

    DrawPolylineClosed(shipShape, 4, ship.pos, ship.angle, shipScale, RAYWHITE);
    if (thrusting) {
      DrawPolylineOpen(flameShape, 5, ship.pos, ship.angle, shipScale,
                       RAYWHITE);
    }

    EndDrawing();
  }

  CloseWindow();
  return 0;
}
