#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "db.h"
#include "accounts.h"

static void set_account_name(struct account_data *account, const char *name)
{
  char tmp[MAX_INPUT_LENGTH];

  if (!account || !name || !*name)
    return;

  strlcpy(tmp, name, sizeof(tmp));
  CAP(tmp);

  if (account->name)
    free(account->name);
  account->name = strdup(tmp);
}

struct account_data *account_create(const char *name)
{
  struct account_data *account;

  CREATE(account, struct account_data, 1);
  set_account_name(account, name);
  return account;
}

struct account_data *account_load(const char *name)
{
  struct account_data *account;
  FILE *fl;
  char filename[PATH_MAX];
  char line[MAX_INPUT_LENGTH + 1];
  char tag[6];

  if (!name || !*name)
    return NULL;

  if (!get_filename(filename, sizeof(filename), ACCT_FILE, name))
    return NULL;

  fl = fopen(filename, "r");
  if (!fl)
    return NULL;

  account = account_create(NULL);

  while (get_line(fl, line)) {
    tag_argument(line, tag);

    if (!strcmp(tag, "Name"))
      set_account_name(account, line);
    else if (!strcmp(tag, "Pass"))
      strlcpy(account->passwd, line, sizeof(account->passwd));
    else if (!strcmp(tag, "Mail")) {
      if (account->email)
        free(account->email);
      account->email = strdup(line);
    } else if (!strcmp(tag, "Char")) {
      if (account->pc_name)
        free(account->pc_name);
      account->pc_name = strdup(line);
    }
  }

  fclose(fl);

  if (!account->name)
    set_account_name(account, name);

  return account;
}

int account_save(const struct account_data *account)
{
  FILE *fl;
  char filename[PATH_MAX];

  if (!account || !account->name || !*account->name)
    return 0;

  if (!get_filename(filename, sizeof(filename), ACCT_FILE, account->name))
    return 0;

  if (!(fl = fopen(filename, "w"))) {
    log("SYSERR: Couldn't open account file %s for write.", filename);
    return 0;
  }

  fprintf(fl, "Name: %s\n", account->name);
  fprintf(fl, "Pass: %s\n", account->passwd);
  if (account->email && *account->email)
    fprintf(fl, "Mail: %s\n", account->email);
  if (account->pc_name && *account->pc_name)
    fprintf(fl, "Char: %s\n", account->pc_name);

  fclose(fl);
  return 1;
}

void account_free(struct account_data *account)
{
  if (!account)
    return;

  if (account->name)
    free(account->name);
  if (account->email)
    free(account->email);
  if (account->pc_name)
    free(account->pc_name);

  free(account);
}

int account_has_alive_pc(const struct account_data *account)
{
  int pfilepos;

  if (!account || !account->pc_name || !*account->pc_name)
    return 0;

  pfilepos = get_ptable_by_name(account->pc_name);
  if (pfilepos < 0)
    return 0;
  if (IS_SET(player_table[pfilepos].flags, PINDEX_DELETED))
    return 0;

  return 1;
}

void account_set_pc(struct account_data *account, const char *pc_name)
{
  char tmp[MAX_INPUT_LENGTH];

  if (!account)
    return;

  if (account->pc_name)
    free(account->pc_name);
  account->pc_name = NULL;

  if (pc_name && *pc_name) {
    strlcpy(tmp, pc_name, sizeof(tmp));
    CAP(tmp);
    account->pc_name = strdup(tmp);
  }
}

void account_clear_pc(struct account_data *account)
{
  if (!account)
    return;

  if (account->pc_name)
    free(account->pc_name);
  account->pc_name = NULL;
}

void account_refresh_pc(struct account_data *account)
{
  if (!account || !account->pc_name || !*account->pc_name)
    return;

  if (account_has_alive_pc(account))
    return;

  account_clear_pc(account);
  account_save(account);
}
