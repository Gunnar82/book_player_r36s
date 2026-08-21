#ifndef PBAP_PHONEBOOK_H
#define PBAP_PHONEBOOK_H

int pbap_phonebook_sync(char book_names[][256], char book_paths[][512], int book_count);
const char *pbap_phonebook_directory(void);

#endif
