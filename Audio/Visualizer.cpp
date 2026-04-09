void Visualizer::Draw(float bass, float mid, float high, bool beat)
{
    float scale = 1.0f + bass * 5.0f;

    if (beat)
        scale += 0.5f;

    glPushMatrix();
    glScalef(scale, scale, 1.0f);

    glBegin(GL_QUADS);

    glColor3f(mid, 0.2f, high);

    glVertex2f(-0.5f, -0.5f);
    glVertex2f(0.5f, -0.5f);
    glVertex2f(0.5f, 0.5f);
    glVertex2f(-0.5f, 0.5f);

    glEnd();

    glPopMatrix();
}