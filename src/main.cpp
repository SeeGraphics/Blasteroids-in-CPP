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
  bool active;
  Vector2 pos;
  Vector2 vel;
  float angle;
  float rotSpeed;
  int sizeLevel;
  int typeId;
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

static float RandRange(float min, float max) {
  float t = GetRandomValue(0, 10000) / 10000.0f;
  return min + (max - min) * t;
}

static float GetAsteroidScale(int sizeLevel) {
  if (sizeLevel <= 0) return 0.35f * 2.0f;
  if (sizeLevel == 1) return 0.6f * 2.0f;
  return 1.0f * 2.0f;
}

static float GetAsteroidRadius(int sizeLevel) {
  const float baseRadius = 18.0f;
  return baseRadius * GetAsteroidScale(sizeLevel);
}

static int SpawnAsteroid(Asteroid* asteroids, int maxAsteroids, Vector2 pos,
                         Vector2 vel, int sizeLevel, int typeId, float angle,
                         float rotSpeed) {
  for (int i = 0; i < maxAsteroids; ++i) {
    if (!asteroids[i].active) {
      asteroids[i].active = true;
      asteroids[i].pos = pos;
      asteroids[i].vel = vel;
      asteroids[i].sizeLevel = sizeLevel;
      asteroids[i].typeId = typeId;
      asteroids[i].angle = angle;
      asteroids[i].rotSpeed = rotSpeed;
      return i;
    }
  }
  return -1;
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
  const int MAX_ASTEROIDS = 12;
  const int startAsteroids = 5;
  const float shipRadius = 12.0f * shipScale;
  const float bulletRadius = 2.0f;
  const float spawnSafeRadius = 140.0f;
  int score = 0;
  bool shipHitThisFrame = false;

  Bullet bullets[MAX_BULLETS];
  for (int i = 0; i < MAX_BULLETS; ++i) {
    bullets[i].active = false;
    bullets[i].pos = {0.0f, 0.0f};
    bullets[i].vel = {0.0f, 0.0f};
    bullets[i].life = 0.0f;
  }

  Asteroid asteroids[MAX_ASTEROIDS];
  for (int i = 0; i < MAX_ASTEROIDS; ++i) {
    asteroids[i].active = false;
    asteroids[i].pos = {0.0f, 0.0f};
    asteroids[i].vel = {0.0f, 0.0f};
    asteroids[i].angle = 0.0f;
    asteroids[i].rotSpeed = 0.0f;
    asteroids[i].sizeLevel = 0;
    asteroids[i].typeId = 0;
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

  const Vector2 asteroidShape0[] = {
      {-12.0f, -8.0f}, {-4.0f, -14.0f}, {6.0f, -10.0f}, {12.0f, -2.0f},
      {8.0f, 10.0f},   {-2.0f, 12.0f},  {-10.0f, 6.0f}, {-14.0f, -2.0f},
  };
  const Vector2 asteroidShape1[] = {
      {-10.0f, -12.0f}, {2.0f, -14.0f}, {12.0f, -6.0f}, {10.0f, 4.0f},
      {4.0f, 12.0f},    {-6.0f, 14.0f}, {-12.0f, 6.0f}, {-14.0f, -2.0f},
  };
  const Vector2 asteroidShape2[] = {
      {-8.0f, -14.0f}, {6.0f, -12.0f}, {14.0f, -4.0f}, {12.0f, 6.0f},
      {4.0f, 14.0f},   {-6.0f, 12.0f}, {-12.0f, 4.0f}, {-12.0f, -6.0f},
  };
  const int asteroidShape0Count =
      sizeof(asteroidShape0) / sizeof(asteroidShape0[0]);
  const int asteroidShape1Count =
      sizeof(asteroidShape1) / sizeof(asteroidShape1[0]);
  const int asteroidShape2Count =
      sizeof(asteroidShape2) / sizeof(asteroidShape2[0]);

  for (int i = 0; i < startAsteroids; ++i) {
    Vector2 pos = {0.0f, 0.0f};
    for (int tries = 0; tries < 50; ++tries) {
      pos.x = RandRange(0.0f, (float)screenWidth);
      pos.y = RandRange(0.0f, (float)screenHeight);
      float dx = pos.x - ship.pos.x;
      float dy = pos.y - ship.pos.y;
      if (dx * dx + dy * dy > spawnSafeRadius * spawnSafeRadius) break;
    }
    float dirAngle = RandRange(0.0f, 2.0f * PI);
    float speed = RandRange(30.0f, 80.0f);
    Vector2 vel = {cosf(dirAngle) * speed, sinf(dirAngle) * speed};
    float rotSpeed = RandRange(-1.5f, 1.5f);
    int typeId = GetRandomValue(0, 2);
    SpawnAsteroid(asteroids, MAX_ASTEROIDS, pos, vel, 2, typeId,
                  RandRange(0.0f, 2.0f * PI), rotSpeed);
  }

  while (!WindowShouldClose()) {
    float dt = GetFrameTime();
    shipHitThisFrame = false;

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

    for (int i = 0; i < MAX_ASTEROIDS; ++i) {
      if (!asteroids[i].active) continue;
      asteroids[i].pos.x += asteroids[i].vel.x * dt;
      asteroids[i].pos.y += asteroids[i].vel.y * dt;
      asteroids[i].pos = WrapPosition(asteroids[i].pos, (float)screenWidth,
                                      (float)screenHeight);
      asteroids[i].angle += asteroids[i].rotSpeed * dt;
    }

    for (int i = 0; i < MAX_BULLETS; ++i) {
      if (!bullets[i].active) continue;
      for (int a = 0; a < MAX_ASTEROIDS; ++a) {
        if (!asteroids[a].active) continue;
        float radius = GetAsteroidRadius(asteroids[a].sizeLevel);
        float dx = bullets[i].pos.x - asteroids[a].pos.x;
        float dy = bullets[i].pos.y - asteroids[a].pos.y;
        float hitDist = radius + bulletRadius;
        if (dx * dx + dy * dy <= hitDist * hitDist) {
          Asteroid parent = asteroids[a];
          bullets[i].active = false;
          asteroids[a].active = false;
          if (parent.sizeLevel > 0) {
            int newSize = parent.sizeLevel - 1;
            float splitAngle = RandRange(0.0f, 2.0f * PI);
            float splitSpeed = RandRange(50.0f, 90.0f);
            Vector2 vel1 = {parent.vel.x + cosf(splitAngle) * splitSpeed,
                            parent.vel.y + sinf(splitAngle) * splitSpeed};
            Vector2 vel2 = {
                parent.vel.x + cosf(splitAngle + PI / 2.0f) * splitSpeed,
                parent.vel.y + sinf(splitAngle + PI / 2.0f) * splitSpeed};
            SpawnAsteroid(asteroids, MAX_ASTEROIDS, parent.pos, vel1, newSize,
                          GetRandomValue(0, 2), RandRange(0.0f, 2.0f * PI),
                          RandRange(-2.0f, 2.0f));
            SpawnAsteroid(asteroids, MAX_ASTEROIDS, parent.pos, vel2, newSize,
                          GetRandomValue(0, 2), RandRange(0.0f, 2.0f * PI),
                          RandRange(-2.0f, 2.0f));
          }
          score += 10;
          break;
        }
      }
    }

    for (int a = 0; a < MAX_ASTEROIDS; ++a) {
      if (!asteroids[a].active) continue;
      float radius = GetAsteroidRadius(asteroids[a].sizeLevel);
      float dx = ship.pos.x - asteroids[a].pos.x;
      float dy = ship.pos.y - asteroids[a].pos.y;
      float hitDist = radius + shipRadius;
      if (dx * dx + dy * dy <= hitDist * hitDist) {
        shipHitThisFrame = true;
        ship.pos = {(float)screenWidth * 0.5f, (float)screenHeight * 0.5f};
        ship.vel = {0.0f, 0.0f};
        break;
      }
    }

    BeginDrawing();
    ClearBackground(BLACK);

    for (int i = 0; i < MAX_ASTEROIDS; ++i) {
      if (!asteroids[i].active) continue;
      const Vector2* shape = asteroidShape0;
      int shapeCount = asteroidShape0Count;
      if (asteroids[i].typeId == 1) {
        shape = asteroidShape1;
        shapeCount = asteroidShape1Count;
      } else if (asteroids[i].typeId == 2) {
        shape = asteroidShape2;
        shapeCount = asteroidShape2Count;
      }
      float scale = GetAsteroidScale(asteroids[i].sizeLevel);
      DrawPolylineClosed(shape, shapeCount, asteroids[i].pos,
                         asteroids[i].angle, scale, RAYWHITE);
    }

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

    DrawText(TextFormat("score: %d", score), 10, 10, 20, RAYWHITE);
    if (shipHitThisFrame) {
      DrawText("ship hit!", 10, 34, 20, RAYWHITE);
    }

    EndDrawing();
  }

  CloseWindow();
  return 0;
}
