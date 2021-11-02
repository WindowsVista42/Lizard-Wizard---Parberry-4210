#include "Math.h"
#include "Defines.h"

Mat4x4 MoveRotateScaleMatrix(Vec3 position, Quat rotation, Vec3 scale) {
    Mat4x4 world = XMMatrixRotationQuaternion(rotation);
    Mat4x4 scl = XMMatrixScaling(scale.x, scale.y, scale.z);
    world = XMMatrixMultiply(scl, world);
    world.r[3] = XMVectorSelect(world.r[3], position, g_XMSelect1110);

    return world;
}

Mat4x4 MoveScaleMatrix(Vec3 position, Vec3 scale) {
    Mat4x4 world = XMMatrixScaling(scale.x, scale.y, scale.z); // scale the world matrix
    world.r[3] = XMVectorSelect(world.r[3], position, g_XMSelect1110); // position the world matrix

    return world;
}

Vec3 JitterVec3(Vec3 input, f32 negativeAccuracy, f32 range) {
    f32 xnoise = range * (GameRandom::Randf32() - 0.5f) * 2.0f;
    f32 ynoise = range * (GameRandom::Randf32() - 0.5f) * 2.0f;
    f32 znoise = range * (GameRandom::Randf32() - 0.5f) * 2.0f;

    return Vec3(
        input.x + xnoise,
        input.y + ynoise,
        input.z + znoise
    );
}

btQuaternion CalculateVelocity(Vec3 origin, Vec3 target, f32 time) {
    // Distance Calculation
    Vec3 distance = target - origin;
    Vec3 distanceXZ = distance;

    // Velocity Calculation
    f32 distanceY = distance.y;
    f32 magnitudeXZ = distanceXZ.x * distanceXZ.x + distanceXZ.y * distanceXZ.y + distanceXZ.z * distanceXZ.z;
    f32 velocityXZ = magnitudeXZ / time;
    f32 velocityY = distanceY / time + 0.5f * abs(-5000.0f) * time;

    // Normalize Vector
    Vec3 resultVector = XMVector3Normalize(distanceXZ);
    resultVector *= velocityXZ;
    resultVector.y = velocityY;

    // Quaternion
    btQuaternion newRotation;
    newRotation.setEulerZYX(resultVector.x, resultVector.y, resultVector.z);

    return newRotation;
}

Vec3 RandomPointInRadius(Vec3 origin, f32 radius) {

    // Randomize
    f32 randomNum = GameRandom::Randf32() * 2 * PI;
    f32 randomPoint = radius * sqrt(randomNum);

    // To Coordinate
    f32 xRand = randomPoint * cos(randomNum);
    f32 zRand = randomPoint * sin(randomNum);

    return Vec3(origin.x + xRand, origin.y, origin.z + zRand);
}