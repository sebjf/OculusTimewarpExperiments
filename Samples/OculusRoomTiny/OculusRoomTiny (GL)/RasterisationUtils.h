#pragma once

// include last!

void CLEAR()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glDisable(GL_TEXTURE_2D);

	glBegin(GL_QUADS);

	glColor4f(1, 0, 0, 1);

	glTexCoord2f(0, 0);
	glVertex2f(-1, -1);

	glTexCoord2f(0, 1);
	glVertex2f(-1, 1);

	glTexCoord2f(1, 1);
	glVertex2f(1, 1);

	glTexCoord2f(1, 0);
	glVertex2f(1, -1);

	glEnd();

	glEnable(GL_TEXTURE_2D);
}
