/*Copyright (C) 
 * 2020 - francisco dot rodriguez at ingenieria dot unam dot mx
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include <stdint.h>
// para int32_t, uint32_t, etc.


//          Esta versión sólo soporta llaves (keys) enteras positivas.


#ifndef PRINT_LEVEL
#define PRINT_LEVEL 0
#endif  

#ifndef DBG_HELP
#define DBG_HELP 1
#endif  

#if DBG_HELP > 0
#define DBG_PRINT( ... ) do{ fprintf( stderr, "DBG:" __VA_ARGS__ ); } while( 0 )
#else
#define DBG_PRINT( ... ) ;
#endif  


enum
{ 
   NOT_FOUND = -3,
   EMPTY_CELL = -2, 
   DELETED_CELL = -1,
};

typedef struct
{
   int32_t bar_code;
   char    name[ 32 ];
   float   price;
} Product;

void Product_Print( Product* p )
{
   printf( 
      "Barcode: %d\n"
      "   Name: %s\n"
      "  Price: $%0.2f\n\n",
      p->bar_code, p->name, p->price );
}


/**
 * Tupla ( key, index )
 */
typedef struct
{
   /**
    * Guarda la llave de indexado. En este ejemplo dicha llave es el código de
    * barras (bar_code). Lo necesitamos para cuando toque resolver colisiones.
    * Aunque en este ejemplo no aplica, la key podría ser negativa, por eso está
    * declarada como entero sin signo.
    */
   int32_t key;

   /**
    * Guarda el índice del producto en la tabla de productos del cliente.
    * EMPTY_CELL indica que la entrada está libre, y DELETED_CELL que la entrada fue borrada. 
    * Aunque los índices siempre son positivos, vamos a utilizar valores
    * negativos como centinelas, por eso está declarada como entero sin signo.
    */
   int32_t index;
} Table_entry;


typedef struct
{
	Table_entry* table; ///< Es la tabla hash
	size_t  size;       ///< Es el número máximo de elementos en la tabla
	size_t  len;        ///< Es el número actual de elementos en la tabla
} Hash_table;



// for debugging purposes only!
static void print_hash_table( const Hash_table* ht )
{
   printf( "----------------------------------------\n" ); 
   printf( "HT.Size: %ld\n", ht->size );
   printf( "HT.Len: %ld\n", ht->len );
   printf( "HT.Table:\n" );
   for( size_t i = 0; i < ht->size; ++i )
   {
      printf( "[%02ld](%d, %d)\n", i, ht->table[ i ].key, ht->table[ i ].index );
   }
   printf( "----------------------------------------\n\n" ); 
}



// Es la función hash
static int h( int key, int m )
{
   return key % m;
}

// es la función de resolución de colisiones
static int probe( int key, int i )
{
   return i + 1;
}



/**
 * @brief Crea una tabla hash.
 *
 * @param size El número de elementos en la tabla hash
 *
 * @return Referencia a una nueva tabla hash.
 */
Hash_table* HT_New( size_t size )
{
   Hash_table* ht = ( Hash_table* )malloc( sizeof( Hash_table ) );
   if( NULL != ht )
   {

      ht->len = 0;
      ht->size = size;

      ht->table = ( Table_entry* ) malloc( size * sizeof( Table_entry ) );
      if( NULL == ht->table )
      {

         free( ht );
         ht = NULL;

      }
      else 
      {
         for( int i = 0; i < ht->size; ++i ) {

            ht->table[ i ].index = EMPTY_CELL;
            ht->table[ i ].key = 0;
         }
      }
   }

   return ht;
   // el cliente es responsable de verificar que efectivamente la tabla se creó
}

/**
 * @brief Destruye una tabla hash.
 *
 * @param ht Referencia a una tabla hash.
 *
 * @post La referencia ht es puesta a NULL.
 */
void HT_Delete( Hash_table** ht )
{
   assert( ht );

   free( (*ht)->table );
   free( *ht );
   *ht = NULL;
}

/**
 * @brief Inserta un elemento en la tabla hash.
 *
 * @param ht Referencia a una tabla hash.
 * @param key El código de barras.
 * @param idx El índice (de la tabla de datos) del elemento a ser insertado.
 *
 * @return 
 */
bool HT_Insert( Hash_table* ht, int32_t key, int32_t idx )
{
   assert( ht );
   assert( ht->len < ht->size );

   int pos;
   // índice que se actualizará en cada colisión

   int home = pos = h( key, ht->size );
   // calcula una hash key base

   DBG_PRINT( "Calculé el valor hash: %d para la llave: %d\n", pos, key );
   // información de depuración


   int i = 0;

   // si el slot está desocupado, se salta el while;
   // en caso contrario entra a buscar uno:
   while( ht->table[ pos ].index >= 0 )
   {
      // no se aceptan duplicados:
      if( ht->table[ pos ].key == key )
      {
         DBG_PRINT( "Error: Llave duplicada\n" );
         return false;
      }

      pos = ( home + probe( key, i ) ) % ht->size;
      DBG_PRINT( "Colisión. Recalculé el valor hash: %d para la llave: %d\n", pos, key );

      ++i;
      // el incremento de esta variable depende del método de resolución de
      // colisiones utilizado
   }

   ht->table[ pos ].key = key;
   ht->table[ pos ].index = idx;

   ++ht->len;

   return true;
}

/**
 * @brief Devuelve el índice idx en la tabla de productos correspondiente al
 * código de barras, key.
 *
 * @param ht Tabla hash
 * @param key El código de barras
 *
 * @return El índice |idx| en la tabla de productos, o\n
 *         EMPTY_CELL si la celda calculada en la tabla hash está vacía, o\n
 *         NOT_FOUND si no se encontró la llave
 *
 * @pre La tabla existe.
 * @pre La tabla no está vacía.
 */
int32_t HT_Search( const Hash_table* ht, int32_t key )
{
   assert( ht );
   assert( ht->len > 0 );

   int home = h( key, ht->size );
   int pos = home;

   DBG_PRINT( "Calculé el valor hash: %d para la llave: %d\n", pos, key );

   int i = 0;

   if( ht->table[ pos ].index == EMPTY_CELL ) return EMPTY_CELL;
   // celda vacía: el producto no existe en la tabla hash

   // si la key en la celda coincide con la key buscada, se salta el while;
   // en caso contrario entra a buscar una coincidencia:
   while( ht->table[ pos ].key != key )
   {    
      pos = ( home + probe( key, i ) ) % ht->size;
      DBG_PRINT( "Celda ocupada: Recalculé el valor hash: %d para la llave: %d\n", pos, key );
      
      ++i;
      // el incremento de esta variable depende del método de resolución de
      // colisiones utilizado
   }

   return( ht->table[ pos ].key == key ? ht->table[ pos ].index : NOT_FOUND );
}

bool HT_IsEmpty( const Hash_table* ht )
{
   return ht->len == 0;
}

bool HT_IsFull( const Hash_table* ht )
{
   return ht->len == ht->size;
}

float HT_LoadFactor( const Hash_table* ht )
{
   return 0.0;
}

#define HASH_TABLE_SIZE 7
#define MAX_PRODUCTS 5


//----------------------------------------------------------------------
// Driver program 
//----------------------------------------------------------------------
int main()
{
   Product productos[ MAX_PRODUCTS ] =
   {
      // Barcode  Name          Price         idx
      { 1010,     "Gansito",     9.0 },    // [0]
      { 2021,     "Crema",      16.5 },    // [1]
      { 3032,     "Arroz",      28.5 },    // ...
      { 4043,     "Papitas",    14.0 },
      { 5054,     "Detergente", 25.0 },
   };

   Hash_table* tabla = HT_New( HASH_TABLE_SIZE );

   assert( tabla );
   // el programa se detiene si la tabla no se pudo crear

   assert( HT_IsEmpty( tabla ) == true );
   // la tabla recién se creó, debe estar vacía

//   print_hash_table( tabla );


   for( size_t i = 0; i < MAX_PRODUCTS && !HT_IsFull( tabla ); ++i )
   {
      HT_Insert( 
         tabla,                   // tabla hash
         productos[ i ].bar_code, // key
         i );                     // idx
   }

   print_hash_table( tabla );

   int32_t idx = HT_Search( tabla, 5054 );
   assert( idx == 4 ); // esperamos el índice 4
   Product_Print( &productos[ idx ] );


   HT_Delete( &tabla );
   assert( tabla == NULL );
}

