#ifndef FRIEND_H 
#define FRIEND_H

#include <string>

void sendFriendRequest(const std::string& senderUsername);

void viewIncomingFriendRequests(const std::string& username);

void viewOutgoingFriendRequests(const std::string& username);

void viewFriends(const std::string& username);

void acceptFriendRequest(const std::string& receiverUsernam);

void rejectFriendRequest(const std::string& receiverUsernam);

void deleteFriend(const std::string& username);


#endif  