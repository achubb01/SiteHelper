#ifndef ACTIONS_H
#define ACTIONS_H

void setBuildSettings(void *context);
void describeStandardStud(void *context);

void addRoom(void *context);
void addWall(void *context);
void generateWall(void *context);
void setWallLength(void *context);
void setStudSpacing(void *context);
void selectRoom(void *context);
void selectWall(void *context);
void describeBuild(void *context);
void printCurrentWall(void *context);
void addOpening(void *context);

#endif