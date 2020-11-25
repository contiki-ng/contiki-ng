#include "tx-power.h"

/*---------------------------------------------------------------------------*/
#define OUTPUT_POWER_UNKNOWN 0xFFFF
int8_t get_tx_power_array_last_element(const tx_power_table_t *array)
{
  uint8_t count = 0;

  while(array->tx_power != OUTPUT_POWER_UNKNOWN) {
    count++;
    array++;
  }
  return count - 1;
}

const tx_power_table_t* rfc_tx_power_last_element(const tx_power_table_t *table){
    int i = get_tx_power_array_last_element(table);
    if (i >= 0)
        return &table[i];
    return NULL;
}

#if RF_TX_POWER_TABLE_STYLE == RF_TX_POWER_TABLE_OLDSTYLE

int8_t rfc_tx_power_min(const tx_power_table_t *table) {
    const tx_power_table_t* x = rfc_tx_power_last_element(table);
    if (x)
        return x->dbm;
    return 0;
}

const tx_power_table_t* rfc_tx_power_eval_power_code(int8_t dbm, const tx_power_table_t *table){
    int i = 0;
    for(; table[i].dbm > dbm; ++i){;}

    if(table[i].tx_power != OUTPUT_POWER_UNKNOWN)
        return &table[i];
    return NULL;
}

#else // RF_TX_POWER_TABLE_SIMPLELINK

int8_t rfc_tx_power_max(const tx_power_table_t *table)
{
    const tx_power_table_t* x = rfc_tx_power_last_element(table);
    if (x)
        return x->dbm;
    return 0;
}


const tx_power_table_t* rfc_tx_power_eval_power_code(int8_t dbm, const tx_power_table_t *table){
    int i = 0;
    for(; table[i].dbm > dbm; ++i){;}

    if(table[i].tx_power != OUTPUT_POWER_UNKNOWN)
        return &table[i];
    return NULL;
/*
    tx_power_table_t* x = NULL;
    for(int i = get_tx_power_array_last_element(); i >= 0; --i) {
      if(dbm > TX_POWER_DRIVER[i].dbm) {
        return &TX_POWER_DRIVER[i];
      }
    }
    return NULL;
*/
}

#endif  //#if RF_TX_POWER_TABLE_STYLE

bool rfc_tx_power_in_range(int8_t dbm, const tx_power_table_t *table)
{
  return (dbm >= rfc_tx_power_min(table)) &&
         (dbm <= rfc_tx_power_max(table));
}
