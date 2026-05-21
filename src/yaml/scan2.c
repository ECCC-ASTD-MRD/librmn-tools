/*
 * Example libyaml parser/decoder.
 *
 * This is a simple libyaml parser example which scans and prints
 * the libyaml parser events.
 *
 */
// make && yamllint fstd2.yaml && ./scan2 ./fstd2.output.yaml  <fstd2.yaml | tee fstd2.listing.txt  && yamllint --no-warnings ./fstd2.output.yaml
// 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <yaml.h>
// #define YAML_STRMOV(DST,SRC) { DST = SRC ; SRC = NULL ; }
YAML_DECLARE(yaml_char_t *) yaml_strdup(const yaml_char_t *);
// YAML_DECLARE(void) yaml_free(void *ptr);
// YAML_DECLARE(void *) yaml_malloc(size_t size);

#define STRVAL(x) ((x) ? (char*)(x) : "")

#define YAML_KEY_VALUE_EVENT 32

// NO-EVENT populated event type
static yaml_event_t null_event = {.type        = YAML_NO_EVENT,
                                  .data.scalar = {NULL, NULL, NULL, 0L, 0, 0, 0},
                                  .start_mark  = {0L, 0L, 0L},
                                  .end_mark    = {0L, 0L, 0L} } ;

// parser/analyzer stack
#define YAML_MAX_STACK_SIZE 128
typedef struct{
  yaml_char_t *anchor[YAML_MAX_STACK_SIZE] ;  // anchor name
  yaml_char_t *symbol[YAML_MAX_STACK_SIZE] ;  // part of data element name associated with this level
  yaml_char_t *value[YAML_MAX_STACK_SIZE] ;   // value of data element at this level
  yaml_char_t *tag[YAML_MAX_STACK_SIZE] ;     // tag of data element at this level
  int32_t level ;                             // stack pointer, identation level
  int32_t event_no[YAML_MAX_STACK_SIZE] ;     // event number in event list (only useful if anchor)
  char mode[YAML_MAX_STACK_SIZE] ;            // M if in mapping mode, S if in sequence mode ( can also be '(', ')', '{',  or '}' )
  int32_t max_size ;
} yaml_stack_t ;

// aliases table (anchor definitions)
#define YAML_MAX_ALIASES 1024
typedef struct{
  int32_t n_aliases ;                     // number of used aliases in table
  int32_t head[YAML_MAX_ALIASES] ;        // index in event list of first event for this alias
  int32_t tail[YAML_MAX_ALIASES] ;        // index in event list of last event for this alias
  yaml_char_t *name[YAML_MAX_ALIASES] ;   // anchor (name of this alias)
  yaml_char_t *value[YAML_MAX_ALIASES] ;  // value if scalar alias (NULL otherwise)
  yaml_char_t *tag[YAML_MAX_ALIASES] ;    // tag if scalar alias (NULL otherwise)
  int32_t max_size ;
} yaml_alias_t ;

// user decoder control
typedef struct yaml_user_decoder_t{
  int32_t version ;                       // user decoder version, used internally by fn
  int32_t data_size ;                     // size of data in bytes, used internally by fn
  int32_t (*fn)(struct yaml_user_decoder_t *decoder, char *symbol, char *tag, char *value, FILE *, int depth) ;
  void *data ;                            // private pointer, used internally by fn
} yaml_user_decoder_t ;
#define DEMO_VERSION 0x0FFFF
yaml_user_decoder_t null_user_decoder = (yaml_user_decoder_t){.version=DEMO_VERSION, .data_size=0, .fn=NULL, .data=NULL } ;  // version 0.ff.ff

// event list
typedef struct{
  yaml_event_t *list ;                    // array of yaml events
  int32_t n_events ;                      // number of events in list
  int32_t max_events ;                    // max number of events that list can contain
} yaml_event_list_t ;

// information for parsing/decoding
typedef struct{
  yaml_stack_t stack ;                    // parsing stack
  yaml_alias_t aliases ;                  // aliases table (anchor values)
  yaml_event_list_t events ;              // event list
  yaml_user_decoder_t decoder ;           // user decoder, application dependent
}yaml_context_t ;

// initialize aliases table
void yaml_aliases_init(yaml_alias_t *table, int max_aliases){
  table->n_aliases = 0 ;                       // no valid aliases
  table->max_size = YAML_MAX_ALIASES ;
  for(int i=0 ; i<YAML_MAX_ALIASES ; i++){
    table->head[i]  = 0 ;                      // no start of anchor
    table->tail[i]  = 0 ;                      // no end of anchor
    table->name[i]  = NULL ;                   // no name
    table->value[i] = NULL ;                   // no value
    table->tag[i]   = NULL ;                   // no tag
  }
}

// initialize parsing stack
void yaml_stack_init(yaml_stack_t *stack, int max_stack){
  stack->level = 0 ;                         // stack is empty
  stack->max_size = YAML_MAX_STACK_SIZE ;
  for(int i=0 ; i<YAML_MAX_STACK_SIZE ; i++){
    stack->event_no[i] = -1 ;                // invalid event number
    stack->mode[i] = ' ' ;                   // invalid mode
    stack->anchor[i] = NULL ;                // no anchor
    stack->symbol[i] = NULL ;                // no symbol
    stack->value[i]  = NULL ;                // no value
    stack->tag[i]    = NULL ;                // no tag
  }
}

#define YAML_MAX_EVENTS 1024
// initialize event list
void yaml_event_list_init(yaml_event_list_t *event_list, int max_events){
  if(max_events <= 0) max_events = YAML_MAX_EVENTS ;
  event_list->n_events = event_list->max_events = 0 ;
  event_list->list = (yaml_event_t *)malloc((max_events+2) * sizeof(yaml_event_t)) ;
  if(event_list->list == NULL) return ;
  event_list->max_events = max_events ;
}

// initialize global control struct for parsing
void yaml_context_init(yaml_context_t *context, int max_stack, int max_aliases, int max_events){
  yaml_stack_init(&(context->stack), max_stack) ;               // initialize the stack
  yaml_aliases_init(&(context->aliases), max_aliases) ;           // initialize the aliases table
  yaml_event_list_init(&(context->events), max_events) ;
}

// push mode token onto parsing stack
// value should be 'M', 'S', '{', or '(' for now
// '{', or '(' is cosmetic
void yaml_stack_push(yaml_stack_t *stack, char value){
  stack->level = stack->level + 1 ;
  stack->mode[stack->level] = value ;
}
// pop parsing stack, return value at top of stack
char yaml_stack_pop(yaml_stack_t *stack){
  char r = stack->mode[stack->level] ;
  stack->level = stack->level - 1 ;
  return r ;
}
// get value at the top of the parsing stack
char yaml_get_stack_top(yaml_stack_t *stack){
  return stack->mode[stack->level] ;
}
// replace the value at the top of the parsing stack
// used to force stack top ot '}' or ')'  (cosmetic)
void yaml_set_stack_top(yaml_stack_t *stack, char value){
  stack->mode[stack->level] = value ;
}

// index  [OUT] : index (position) in aliases table
// head   [OUT] : index in list of first event for this alias/anchor
// tail   [OUT] : index in list of last event for this alias/anchor
// aliases [IN] : aliases table (normally from context struct)
// anchor  [IN] : anchor to be substituted
// return associated value if any, index/head/tail set to -1 in case of error
yaml_char_t *yaml_alias_range(int *index, int *head, int *tail, yaml_alias_t *aliases, yaml_char_t *anchor){
  yaml_char_t *target = anchor ;
  *head = *tail = *index = -1 ;
  for(int i = 0 ; i < aliases->n_aliases ; i++){
    if( 0 == strncmp((char *)target, (char *)aliases->name[i], 1024) ){
      *head = aliases->head[i] ;
      *tail = aliases->tail[i] ;
      *index = i ;
      return aliases->value[i] ;
    }
  }
  return NULL ;  // not found or no scalar value associated with alias
}

static FILE *parsed_out = NULL ;

static char _spaces[] = "                                                                      " ;
// return pointer to a string of n spaces ( n < sizeof(_spaces) )
char *spaces(int n){
  if(n <= 0) return "" ;
  if(n >= sizeof(_spaces)) return _spaces ;
  return  _spaces + (sizeof(_spaces) - n - 1) ;
}

// print event description header
// stack    [IN] : stack pointer
// old_tos  [IN] : old top of stack (before this event)
// depth    [IN] : indentation level (number of spaces at end of header)
// event_id [IN] : possibly modified event number
static void print_header(yaml_stack_t *stack, char old_tos, int depth, int event_id)
{
    char tos = yaml_get_stack_top(stack) ;
    if(event_id < 0){                      // event inserted during alias processing
      printf("->%4.4d ", -event_id) ;
    }else{                                 // normal event
      printf("  %4.4d ", event_id) ;
    }
    printf("%c->%c|%2.2d|:[%2d]",old_tos, tos, stack->level, depth);
    printf("%s", spaces(depth)) ;
}

//TODO add tag to the fray
void print_parsed(yaml_stack_t *stack, FILE *f, int depth, char *key, char *value, char *tag){
  if(yaml_get_stack_top(stack) == 'S'){
    fprintf(f, "%s%s%s\n", spaces((depth-3)*2), key[0] ? "- " : "-", key) ;
    return ;
  }
  if(yaml_get_stack_top(stack) == 'M'){
    if(strncmp(key, "<<", 2) == 0) return ;   // ignore << alias insertion marker
    fprintf(f, "%s%s:", spaces((depth-3)*2), key) ;   // key:
    if(value[0]){
      if(tag[0]) fprintf(f, " %s", tag) ;             // tag if present
      fprintf(f, " %s", value) ;                      // value
    }
    fprintf(f, "\n") ;
//     fprintf(f, "%s%s:%s%s\n", spaces((depth-3)*2), key, value[0] ? " " : "", value) ;
    return ;
  }
}

// print symbol/tag/value trio
void print_stv(FILE *f, int depth, char *symbol, char *tag, char *value){
    fprintf(f, "%s", (depth == 0) ? "#       " : "." ) ;    // "#      " first time around. "." afterwards
  if(value){
    if( value[0] == '\0' ) value = "' '" ;
    if(tag != NULL){
      fprintf(f, "%s = (%s)%s", symbol, tag+1, value) ;
    }else{
      fprintf(f, "%s = %s", symbol,value) ;
    }
  }else{
    if(tag != NULL){
      fprintf(f, "(%s)%s", tag+1, symbol) ;
    }else{
      fprintf(f, "%s", symbol) ;
    }
  }
}

int32_t demo_user_decode(yaml_user_decoder_t *decoder, char *symbol, char *tag, char *value, FILE *f, int depth){
  if(decoder->fn != demo_user_decode) exit(1) ;
  if(decoder->version != DEMO_VERSION) exit(1) ;
  fprintf(f, "%s", (depth == 0) ? "# DEMO " : "" ) ;    // "# DEMO " first time around, nothing afterwards
  print_stv(f, depth, symbol, tag, value) ;
  return 0 ;
}

#define MAX_DEPTH 128
// process symbols on stack
int32_t process_symbol_stack(yaml_context_t *context, FILE *f){
  yaml_stack_t *stack = &(context->stack) ;
  int i, status = 0 ;
  char *symbols[MAX_DEPTH], *tags[MAX_DEPTH], *values[MAX_DEPTH] ;
  int depth = 0 ;

  // do nothing if "<<" is found at any level
  for(i = 0 ; i <= stack->level ; i++){
    char *symbol = (char *)stack->symbol[i] ;
    if(symbol == NULL) continue ;                  // no symbol string
    if(symbol[0] == 0) continue ;                  // symbol is null string
    if( strncmp(symbol, "<<", 2) == 0 ) goto end ;   // <<: present

    symbols[depth] = (char *)stack->symbol[i] ;    // collect symbols, tags, and values
    tags[depth]    = (char *)stack->tag[i] ;
    values[depth]  = (char *)stack->value[i] ;
    depth++ ;
  }
  // process collected symbol, tag, and value trios
  for(i = 0 ; i < depth ; i++){
    if(context->decoder.fn == NULL){
//       fprintf(f, "%s", (i == 0) ? "# SYMBOL " : "." ) ;    // "#      " first time around. "." afterwards
      print_stv(f, i, symbols[i], tags[i], values[i]) ;     // process symbol/tag/value trio
    }else{
//       fprintf(f, "%s", (i == 0) ? "# DEMO   " : "." ) ;    // "#      " first time around. "." afterwards
//       print_stv(f, symbols[i], tags[i], values[i]) ;     // process symbol/tag/value trio
      status = (*(context->decoder.fn))(&(context->decoder), symbols[i], tags[i], values[i], f, i) ;
    }
  }
  fprintf(f, "\n") ;
end:
  return status ;
}

#define DEBUG
#if defined(DEBUG)
#define CHECK_YAML_STACK if(level != stack->level) exit(1)
#else
#define CHECK_YAML_STACK
#endif

#define PRINT_INDENT(SPACES) print_header(stack, tos, SPACES, event_id)
// context [INOUT] : information used for parsing
// event_no   [IN] : index in list of event being processed
// event_id   [IN] : pseudo event number, only used in diagnostic prints
// list       [IN] : event list (array of events)
// TODO: suppress list and get it from context
int process_event(yaml_context_t *context, int event_no, int event_id) // , yaml_event_t *list)
{
    yaml_event_t *list = ( yaml_event_t *)context->events.list ;
    yaml_alias_t *aliases = &(context->aliases) ;   // anchor name and position table
    yaml_stack_t *stack = &(context->stack) ;       // mode stack (sequence | mapping | stream | document)
    char tos = yaml_get_stack_top(stack) ;          // current top of mode stack (only used in diagnostic prints)
    yaml_event_t *event = &list[event_no] ;                                      // event being processed
    yaml_event_t *next_event = event + 1 ;                                       // following event
    yaml_event_t *prev_event = (event_no == 0) ? (&null_event) : (event - 1) ;   // preceding event
    int level = stack->level ;                      // current level in mode stack
    int process_symbols = 0 ;
    int head = -1, tail = -1, index = -1 ;

    CHECK_YAML_STACK ;
    switch ((int)event->type) {
    case YAML_NO_EVENT:
#if defined(FULL_DEBUG)
        PRINT_INDENT(level);
        printf("NO-OP (%d)\n", event->type);
#endif
        break;

    case YAML_STREAM_START_EVENT:
        yaml_stack_push(stack, '{') ; level++ ;
        PRINT_INDENT(level - 1);
        printf("STREAM_START (%d)\n", event->type);
        yaml_set_stack_top(stack, '}') ;
        break;

    case YAML_STREAM_END_EVENT:
        yaml_stack_pop(stack); level-- ;
        PRINT_INDENT(level);
        printf("STREAM_END (%d)\n", event->type);
        break;

    case YAML_DOCUMENT_START_EVENT:
        yaml_stack_push(stack, '(') ; level++ ;
        PRINT_INDENT(level - 1);
        printf("DOC_START (%d)\n", event->type);
        yaml_set_stack_top(stack, ')') ;
        break;

    case YAML_DOCUMENT_END_EVENT:
        yaml_stack_pop(stack); level-- ;
        PRINT_INDENT(level);
        printf("DOC_END (%d)\n", event->type);
        break;

    case YAML_ALIAS_EVENT:
        PRINT_INDENT(level);
        yaml_alias_range(&index, &head, &tail, aliases, event->data.alias.anchor) ;
        printf("ALIAS[%d] (%d), {anchor=\"%s\"}, events[%4.4d:%4.4d]\n",
               event_id,
               event->type,
               STRVAL(event->data.alias.anchor),
               head, tail
              );
        // if alias was a scalar alias (head == tail), alias event should have already been replaced with key/value event
        // key from previous event, value from alias table
        if(tail == head){
          PRINT_INDENT(level);
          printf("ERROR: SCALAR alias encountered\n");
        }else{
          // if anchor name is preceded by <<:, eliminate first and last event in what anchor points to
          if(prev_event->type == YAML_SCALAR_EVENT){
            if( strncmp((char *)prev_event->data.scalar.value, "<<", 2) == 0 ){
              head++ ;
              tail-- ;
            }
          }
          for(int i = head ; i <= tail ; i++){    // inject event sequence pointed to by anchor
            process_event(context, i, -i) ; // head <= event number <= tail
          }
        }
        break;

    case YAML_KEY_VALUE_EVENT:
        PRINT_INDENT(level);
        printf("KEY_VALUE[%d] (%d) = {key=\"%s\", value=\"%s\", length=%d}, tag=\"%s\"\n",
              event_id,
              event->type,
              STRVAL(event->data.scalar.anchor),
              STRVAL(event->data.scalar.value),
              (int)event->data.scalar.length,
              STRVAL(event->data.scalar.tag)  );
        print_parsed(stack, parsed_out, level,
                     STRVAL(event->data.scalar.anchor),
                     STRVAL(event->data.scalar.value),
                     STRVAL(event->data.scalar.tag)) ;
        // insert key=value as symbol[level]
        stack->symbol[level] = yaml_strdup( event->data.scalar.anchor ) ;
        stack->value[level] = yaml_strdup( event->data.scalar.value ) ;
        stack->tag[level] = yaml_strdup( event->data.scalar.tag ) ;
        stack->symbol[level+1] = stack->value[level+1] = stack->tag[level+1] = NULL ;
        process_symbols = 1 ;
        break;

    case YAML_SCALAR_EVENT:
        // ====================================================================================================
        // if next event is a scalar ALIAS, next event will become a SCALAR event, value taken from alias table
        if(next_event->type == YAML_ALIAS_EVENT)
        {
          PRINT_INDENT(level);
          char *value = (char *)yaml_alias_range(&index, &head, &tail, aliases, next_event->data.alias.anchor) ;
          printf("SCALAR_ALIAS '|%s|%s|' FOLLOWS, head,tail = %d,%d, entry = '|%s|%s|%s|'\n",
                 next_event->data.alias.anchor,
                 value ? value : "NoNe",
                 head, tail, aliases->name[index], aliases->value[index], aliases->tag[index] ) ;
          if(value)    // there is a value associated with the alias in next event
          {
            yaml_event_t et ;
            et.type = YAML_SCALAR_EVENT ;
            et.data.scalar.value = (yaml_char_t *)value ;
            et.data.scalar.length = strnlen(value, 1024) ;
            et.data.scalar.anchor = NULL ;
            et.data.scalar.tag = aliases->tag[index] ;
            et.data.scalar.plain_implicit = event->data.scalar.plain_implicit ;
            et.data.scalar.quoted_implicit = event->data.scalar.quoted_implicit ;
            *next_event = et ;  // alias -> scalar, value coming from alias table
          }
        }
        // ====================================================================================================
        // SCALAR event followed by event with ANCHOR, insert anchor into aliases table
        if(next_event->type == YAML_SCALAR_EVENT && next_event->data.scalar.anchor != NULL)
        {
          aliases->name[aliases->n_aliases] = next_event->data.scalar.anchor ;
          aliases->value[aliases->n_aliases] = yaml_strdup(next_event->data.scalar.value) ;
          aliases->tag[aliases->n_aliases] = (yaml_char_t *)STRVAL(next_event->data.scalar.tag) ;
          next_event->data.scalar.anchor = NULL ;
          aliases->head[aliases->n_aliases] = event_no + 1 ;   // start point
          aliases->tail[aliases->n_aliases] = event_no + 1 ;   // end = start for scalar
          PRINT_INDENT(level);
          printf("SCALAR_ANCHOR = '%s|%s|%s' at %d\n",
                 aliases->name[aliases->n_aliases],
                 aliases->value[aliases->n_aliases],
                 aliases->tag[aliases->n_aliases],
                 event_no + 1) ;
          aliases->n_aliases = aliases->n_aliases + 1 ;
        }
        // ====================================================================================================
        // should next event be transformed into key = value event (YAML_KEY_VALUE_EVENT) ?
        if(next_event->type == YAML_SCALAR_EVENT   &&     // next event is SCALAR
           yaml_get_stack_top(stack) == 'M' &&            // we are in mapping mode
           next_event->data.scalar.anchor == NULL)        // next event is not an anchor
        {
          next_event->data.scalar.anchor = event->data.scalar.value ;  // current value becomes key for next event
          event->data.scalar.value = NULL ;           // make sure value string does not get freed when disposing of event
          next_event->type = YAML_KEY_VALUE_EVENT ;   // next event is key = value type (fudged YAML event type)
          event->type = YAML_NO_EVENT ;               // make current event a NO-OP
          break;                                      // do not print nor parse, we are done
        }
        // ====================================================================================================
        // SCALAR event  not followed by MAPPING or ALIAS
        if(next_event->type != YAML_MAPPING_START_EVENT &&  next_event->type != YAML_ALIAS_EVENT)
        {
          print_parsed(stack, parsed_out, level,
                       STRVAL(event->data.scalar.value),
                       "",
                       STRVAL(event->data.scalar.tag)) ;  // single value
          // insert value into stack symbol[level]
          stack->symbol[level] = yaml_strdup( event->data.scalar.value ) ;
          stack->tag[level]    = yaml_strdup( event->data.scalar.tag ) ;
          stack->value[level]  = NULL ;
          stack->symbol[level+1] = stack->value[level+1] = stack->tag[level+1] = NULL ;  // nullify next level
          process_symbols = 1 ;
        }
        // ====================================================================================================
        PRINT_INDENT(level);
        printf("SCALAR[%d] (%d) = {value=\"%s\", length=%d}, anchor=\"%s\", tag=\"%s\"\n",
              event_id,
              event->type,
              STRVAL(event->data.scalar.value),
              (int)event->data.scalar.length,
              STRVAL(event->data.scalar.anchor),
              STRVAL(event->data.scalar.tag)  );
        // ====================================================================================================
        break;

    case YAML_SEQUENCE_START_EVENT:
        yaml_stack_push(stack, 'S') ; level++ ;
        PRINT_INDENT(level - 1);
        if(event->data.sequence_start.anchor){
          stack->anchor[level] = event->data.sequence_start.anchor ;
          event->data.sequence_start.anchor = NULL ;
          stack->event_no[level] = event_no ;           // anchor range starts here
          printf("SEQUENCE_ANCHOR_START(%d) : anchor='%s' at event %d\n",
                 level,
                 stack->anchor[level],
                 stack->event_no[level]) ;
        }else{
          printf("SEQUENCE_START[%d] (%d), tag=\"%s\"\n",
                event_id,
                event->type,
                STRVAL(event->data.sequence_start.tag));
        }
        break;

    case YAML_SEQUENCE_END_EVENT:
        PRINT_INDENT(level - 1);
        if( stack->anchor[level] ){
          printf("SEQUENCE_ANCHOR_END(%d) : '%s', events %d to %d\n",
                 level, stack->anchor[level], stack->event_no[level], event_id) ;
          aliases->name[aliases->n_aliases] = stack->anchor[level] ;
          aliases->value[aliases->n_aliases] = NULL ;                        // not a scalar anchor
          aliases->tag[aliases->n_aliases] = NULL ;                          // not a scalar anchor
          stack->anchor[level] = NULL ;
          aliases->head[aliases->n_aliases] = stack->event_no[level] ;       // remembered start point
          aliases->tail[aliases->n_aliases] = event_no ;                     // anchor range ends here
          aliases->n_aliases = aliases->n_aliases + 1 ;
        }else{
          printf("SEQUENCE_END[%d] (%d)\n",
                event_id,
                event->type);
        }
        yaml_stack_pop(stack); level-- ;
        break;

    case YAML_MAPPING_START_EVENT:
        PRINT_INDENT(level);
        char *name="" ;
        // if previous event was SCALAR, mapping name comes from its value
        if(prev_event->type == YAML_SCALAR_EVENT) name = (char *)prev_event->data.scalar.value ;
        name = name ? name : "NULL" ;
        if(event->data.mapping_start.anchor){
          print_parsed(stack, parsed_out, level, name, "", "") ;
          yaml_stack_push(stack, 'M') ; level++ ;
          stack->anchor[level] = event->data.mapping_start.anchor ;
          stack->symbol[level-1] = yaml_strdup( (yaml_char_t *)name ) ;
          stack->value[level-1] = stack->tag[level-1] = NULL ;
          stack->symbol[level] = stack->value[level] = stack->tag[level] = NULL ;
          event->data.mapping_start.anchor = NULL ;
          stack->event_no[level] = event_no ;     // anchor range starts at this event
          printf("MAPPING_ANCHOR_START(%d) : name='%s', anchor='%s' at event %d\n",
                 level,
                 name,
                 stack->anchor[level],
                 stack->event_no[level]) ;
        }else{
          printf("MAPPING_START[%d] (%d), name='%s', anchor=\"%s\", tag=\"%s\"\n",
                event_id,
                event->type,
                 name,
                STRVAL(event->data.mapping_start.anchor),
                STRVAL(event->data.mapping_start.tag));
          print_parsed(stack, parsed_out, level, name, "", "") ;
          stack->symbol[level] = yaml_strdup( (yaml_char_t *)name ) ;
          stack->value[level] = stack->tag[level] = NULL ;
          stack->symbol[level+1] = stack->value[level+1] = stack->tag[level+1] = NULL ;
          yaml_stack_push(stack, 'M') ; level++ ;
        }
        process_symbols = 1 ;
        break;

    case YAML_MAPPING_END_EVENT:
        PRINT_INDENT(level - 1);
        if( stack->anchor[level] ){
          printf("MAPPING_ANCHOR_END(%d) : '%s', events %d to %d\n",
                 level, stack->anchor[level],
                 stack->event_no[level],
                 event_no - 1) ;
//           aliases->name[aliases->n_aliases] = (char *)yaml_strdup(stack->anchor[level]) ;
//           free(stack->anchor[level]) ;
          aliases->name[aliases->n_aliases] = stack->anchor[level] ;
          aliases->value[aliases->n_aliases] = NULL ;                        // not a scalar anchor
          aliases->tag[aliases->n_aliases] = NULL ;                          // not a scalar anchor
          stack->anchor[level] = NULL ;
          aliases->head[aliases->n_aliases] = stack->event_no[level] ;       // remembered start point
          aliases->tail[aliases->n_aliases] = event_no ;                     // anchor range ends here
          aliases->n_aliases = aliases->n_aliases + 1 ;
        }else{
          printf("MAPPING_END[%d] (%d)\n",
                event_id,
                event->type);
        }
        yaml_stack_pop(stack); level-- ;
        break;

    default:
        printf("OTHER (%d)\n", event->type);
        break;
    }
    CHECK_YAML_STACK ;

    if (level < 0) {
        printf("ERROR: indentation underflow!\n");
        return 1 ;
    }
    if(process_symbols) process_symbol_stack(context, parsed_out) ;

    return 0 ;
}

int parse_event_list(yaml_context_t *context){
  yaml_event_t *list = context->events.list ;
  yaml_event_t event ;
  yaml_event_type_t event_type ;
  int event_no = 0 ;

  do{
    event = list[event_no] ;
    event_type = event.type ;
    if( process_event(context, event_no, event_no) ) return 0 ;
    event_no++ ;
  }while (event_type != YAML_STREAM_END_EVENT) ;

  return (event_type == YAML_STREAM_END_EVENT) ;
}

int get_event_list(yaml_context_t *context, yaml_parser_t *parser){
  int event_no ;
  int max_events = context->events.max_events ;
  yaml_event_type_t event_type ;
  yaml_event_t *events = context->events.list ;

  event_type = YAML_NO_EVENT ;
  for(event_no=0 ; (event_no < max_events) && (event_type != YAML_STREAM_END_EVENT) ; event_no++){
    if( !yaml_parser_parse(parser, &events[event_no]) ) goto error ;
    event_type = events[event_no].type ;
  }
  if(event_type != YAML_STREAM_END_EVENT) goto error ;   // premature end of list
  events[event_no] = null_event ;

  context->events.n_events = event_no+1 ;
  return event_no ;    // return number of events

error:
  printf("ERROR: getting event list\n") ;
  return -1 ;
}

int main(int argc, char *argv[])
{
    int event_no ;
    char *filename = (argc > 1) ? argv[1] : "stderr" ;
    unsigned char file_buffer[1024*1024] ;
    ssize_t file_buffer_size = 0 ;

    yaml_context_t yaml_context, *yaml_context_p = &yaml_context ;
    yaml_alias_t *aliases = &(yaml_context.aliases) ;
    yaml_parser_t parser;

    if(argc > 1){
      parsed_out = fopen(filename, "w+") ;
      if(parsed_out == NULL) exit(1) ;
    }else{
      parsed_out = stderr ;
    }
    fprintf(parsed_out, "---\n") ;

    yaml_parser_initialize(&parser);
//     yaml_parser_set_input_file(&parser, stdin);
    // read data from stdin, set buffer as YAML source
    file_buffer_size = read(0, file_buffer, sizeof(file_buffer)) ;
    if(file_buffer_size <= 0) exit(1) ;
    yaml_parser_set_input_string(&parser, file_buffer, (size_t)file_buffer_size) ;

    // create context
    yaml_context_init(yaml_context_p, YAML_MAX_STACK_SIZE, YAML_MAX_ALIASES, YAML_MAX_EVENTS) ;
    yaml_context.decoder = null_user_decoder ;
    // initialize user decoder
    yaml_context.decoder = null_user_decoder ;
    if(getenv("YAML_DEMO") != NULL) yaml_context.decoder.fn = demo_user_decode ;
    // populate event list
    if( (event_no = get_event_list(yaml_context_p, &parser)) <= 0) goto error ;
    // parse event list
    if( !parse_event_list(yaml_context_p) ) goto error2 ;

    yaml_parser_delete(&parser);    // delete libyaml parser after use

    printf("'%s' used for parsed YAML output\n", filename) ;
    printf("number of events = %d\n", event_no) ;
    printf("sizeof(yaml_event_t) = %ld\n", sizeof(yaml_event_t)) ;
    fprintf(parsed_out, "...\n") ;
    fclose(parsed_out) ;
    printf("alias table\n") ;
    for(int i=0 ; i<aliases->n_aliases ; i++){
      printf("%3d : [%4.4d:%4.4d] anchor='%20s', value='%20s', tag='%20s'\n", i, aliases->head[i], aliases->tail[i],  aliases->name[i],  aliases->value[i], aliases->tag[i]) ;
    }
    return EXIT_SUCCESS;

error:
    printf("ERROR: Failed to parse: %s\n", parser.problem);
    yaml_parser_delete(&parser);
    return EXIT_FAILURE;

error2:
    printf("ERROR: analyzing event list\n") ;
    return EXIT_FAILURE;
}
