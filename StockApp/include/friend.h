#ifndef FRIEND_H 
#define FRIEND_H

#include <string>

void sendFriendRequest(int userID);

void viewIncomingFriendRequests(int userID);

void viewOutgoingFriendRequests(int userID);

void viewFriends(int userID);

void acceptFriendRequest(int userID);

void rejectFriendRequest(int userID);

void deleteFriend(int userID);


#endif  