/**
* @file species.h
* Race/species related header.
* 
* This set of code was not originally part of the circlemud distribution.
*/

#ifndef _SPECIES_H_
#define _SPECIES_H_

struct char_data;

const char *get_species_name(int species);
int species_from_pc_choice(int choice);
int pc_species_count(void);
bool species_is_pc_selectable(int species);
int species_ability_mod(int species, int ability);
int species_ability_min(int species, int ability);
int species_ability_cap(int species, int ability);

void apply_species_bonuses(struct char_data *ch);
void grant_species_skills(struct char_data *ch);
int parse_species(const char *arg);
void update_species(struct char_data *ch, int new_species);

extern const char *species_types[];
extern const int pc_species_list[];

#endif /* _SPECIES_H_ */
