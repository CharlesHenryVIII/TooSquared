#include "Math.h"
#include <cmath>

void Swap(void* a, void* b, const int size)
{

    char* c = (char*)a;
    char* d = (char*)b;
    for (int i = 0; i < size; i++)
    {

		char temp = c[i];
		c[i] = d[i];
		d[i] = temp;
    }
}

Vector CreateRandomVector(const Vector& min, const Vector& max)
{
    return { Random<float>(min.x, max.x),
             Random<float>(min.y, max.y) };
}

Color CreateRandomColor(Color min, Color max)
{
    return { Random(min.r, max.r),
            Random(min.g, max.g),
            Random(min.b, max.b),
            Random(min.a, max.a) };
}

Color CreateRandomColorShade(float min, float max)
{
	float r = Random(min, max);
	return { r, r, r, r };
}

float LinearToAngularVelocity(Vector centerOfCircle, Vector position, Vector velocity)
{

	Vector toCenter = centerOfCircle - position;
	Vector tangent = { toCenter.y, -toCenter.x };
	float velocityScale = DotProduct(NormalizeZero(tangent), NormalizeZero(velocity));
	float radius = Distance(centerOfCircle, position);
	return Pythags(velocity) * velocityScale / (tau * radius);
}

gbVec3 Vec2TogbVec3(Vector v)
{
	return { v.x, v.y, 0.0f };
}

Vector gbMat4ToVec2(gbMat4 m)
{
	return { m.col[3].xy.x, m.col[3].xy.y };
}

Vector RotateVector(Vector v, float deg)
{
	gbMat4 rotatedMat;
	gb_mat4_rotate(&rotatedMat, { 0, 0, 1 }, DegToRad(deg));

	gbMat4 translateMat;
	gb_mat4_translate(&translateMat, Vec2TogbVec3(v));

	return gbMat4ToVec2(rotatedMat * translateMat);
}
