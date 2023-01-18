﻿// Copyright (c) 2023 Evangelion Manuhutu | ORigin Engine

namespace ORiginEngine
{
    public struct Vector2
    {
        public float X, Y;

        public Vector2(float scalar)
        {
            X = scalar;
            Y = scalar;
        }

        public Vector2(float x, float y)
        {
            X = x;
            Y = y;
        }

        public static Vector2 Zero => new Vector2(0.0f);

        public static Vector2 operator* (Vector2 vector, float scalar)
        {
            return new Vector2(vector.X * scalar, vector.Y * scalar);
        }

        public static Vector2 operator/ (Vector2 vector, float scalar)
        {
            return new Vector2(vector.X / scalar, vector.Y / scalar);
        }

        public static Vector2 operator+ (Vector2 vectorA, Vector2 vectorB)
        {
            return new Vector2(vectorA.X + vectorB.X, vectorA.Y + vectorB.Y);
        }

        public static Vector2 operator- (Vector2 vectorA, Vector2 vectorB)
        {
            return new Vector2(vectorA.X - vectorB.X, vectorA.Y - vectorB.Y);
        }
    }
}