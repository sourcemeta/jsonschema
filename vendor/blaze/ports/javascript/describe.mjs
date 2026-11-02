import {
  ASSERTION_FAIL, ASSERTION_DEFINES, ASSERTION_DEFINES_STRICT,
  ASSERTION_DEFINES_ALL, ASSERTION_DEFINES_ALL_STRICT,
  ASSERTION_DEFINES_EXACTLY, ASSERTION_DEFINES_EXACTLY_STRICT,
  ASSERTION_DEFINES_EXACTLY_STRICT_HASH3, ASSERTION_PROPERTY_DEPENDENCIES,
  ASSERTION_TYPE, ASSERTION_TYPE_ANY, ASSERTION_TYPE_STRICT,
  ASSERTION_TYPE_STRICT_ANY, ASSERTION_NOT_TYPE_STRICT_ANY,
  ASSERTION_TYPE_STRING_BOUNDED,
  ASSERTION_TYPE_STRING_UPPER, ASSERTION_TYPE_ARRAY_BOUNDED,
  ASSERTION_TYPE_ARRAY_UPPER, ASSERTION_TYPE_OBJECT_BOUNDED,
  ASSERTION_TYPE_OBJECT_UPPER, ASSERTION_REGEX,
  ASSERTION_STRING_SIZE_LESS, ASSERTION_STRING_SIZE_GREATER,
  ASSERTION_ARRAY_SIZE_LESS, ASSERTION_ARRAY_SIZE_GREATER,
  ASSERTION_OBJECT_SIZE_LESS, ASSERTION_OBJECT_SIZE_GREATER,
  ASSERTION_EQUAL, ASSERTION_EQUALS_ANY, ASSERTION_EQUALS_ANY_STRING_HASH,
  ASSERTION_GREATER_EQUAL, ASSERTION_LESS_EQUAL,
  ASSERTION_GREATER, ASSERTION_LESS,
  ASSERTION_UNIQUE, ASSERTION_DIVISIBLE,
  ASSERTION_TYPE_INTEGER_BOUNDED, ASSERTION_TYPE_INTEGER_BOUNDED_STRICT,
  ASSERTION_TYPE_INTEGER_LOWER_BOUND, ASSERTION_TYPE_INTEGER_LOWER_BOUND_STRICT,
  ASSERTION_NUMBER_INTEGER_BOUNDED,
  ASSERTION_NUMBER_TYPE,
  ASSERTION_STRING_TYPE,
  ASSERTION_PROPERTY_TYPE, ASSERTION_PROPERTY_TYPE_EVALUATE,
  ASSERTION_PROPERTY_TYPE_STRICT, ASSERTION_PROPERTY_TYPE_STRICT_EVALUATE,
  ASSERTION_PROPERTY_TYPE_STRICT_ANY, ASSERTION_PROPERTY_TYPE_STRICT_ANY_EVALUATE,
  ASSERTION_PROPERTY_EQUAL,
  ASSERTION_ARRAY_PREFIX, ASSERTION_ARRAY_PREFIX_EVALUATE,
  ASSERTION_OBJECT_PROPERTIES_SIMPLE,
  ANNOTATION_EMIT, ANNOTATION_EMIT_WRAPPED, ANNOTATION_TO_PARENT, ANNOTATION_BASENAME_TO_PARENT,
  EVALUATE,
  LOGICAL_NOT, LOGICAL_NOT_EVALUATE,
  LOGICAL_OR, LOGICAL_AND, LOGICAL_XOR, LOGICAL_CONDITION,
  LOGICAL_WHEN_TYPE, LOGICAL_WHEN_DEFINES, LOGICAL_WHEN_ARRAY_SIZE_GREATER,
  LOOP_PROPERTIES_UNEVALUATED, LOOP_PROPERTIES_UNEVALUATED_EXCEPT,
  LOOP_PROPERTIES_MATCH, LOOP_PROPERTIES_MATCH_CLOSED,
  LOOP_PROPERTIES, LOOP_PROPERTIES_EVALUATE,
  LOOP_PROPERTIES_REGEX, LOOP_PROPERTIES_REGEX_CLOSED,
  LOOP_PROPERTIES_STARTS_WITH, LOOP_PROPERTIES_EXCEPT,
  LOOP_PROPERTIES_TYPE, LOOP_PROPERTIES_TYPE_EVALUATE,
  LOOP_PROPERTIES_EXACTLY_TYPE_STRICT, LOOP_PROPERTIES_EXACTLY_TYPE_STRICT_HASH,
  LOOP_PROPERTIES_TYPE_STRICT, LOOP_PROPERTIES_TYPE_STRICT_EVALUATE,
  LOOP_PROPERTIES_TYPE_STRICT_ANY, LOOP_PROPERTIES_TYPE_STRICT_ANY_EVALUATE,
  LOOP_KEYS, LOOP_ITEMS, LOOP_ITEMS_FROM, LOOP_ITEMS_UNEVALUATED,
  LOOP_ITEMS_TYPE, LOOP_ITEMS_TYPE_STRICT, LOOP_ITEMS_TYPE_STRICT_ANY,
  LOOP_ITEMS_PROPERTIES_EXACTLY_TYPE_STRICT_HASH,
  LOOP_ITEMS_PROPERTIES_EXACTLY_TYPE_STRICT_HASH3,
  LOOP_ITEMS_INTEGER_BOUNDED, LOOP_ITEMS_INTEGER_BOUNDED_SIZED,
  LOOP_ITEMS_ARRAY_BOUNDED_TYPE_STRICT_ANY,
  LOOP_CONTAINS,
  CONTROL_DYNAMIC_ANCHOR_JUMP, CONTROL_JUMP
} from './opcodes.mjs';

const TYPE_INTEGER = 2;
const TYPE_REAL = 3;

const TYPE_NAMES = [ 'null', 'boolean', 'integer', 'number',
                     'string', 'array', 'object', 'number' ];

// The sentences themselves live on the compiled template, which the C++
// compiler resolved one per instruction. What is left here are the phrases
// that fill them, as those depend on the instance
const UNKNOWN = '<unknown>';

// The arguments every annotation message draws on, in the order the compiler
// wrote them into its templates
const ANNOTATION_SCOPE = 0;
const ANNOTATION_QUOTED = 1;
const ANNOTATION_SPELLED = 2;
const ANNOTATION_LIST = 3;
const ANNOTATION_KEYWORD = 4;
const ANNOTATION_JSON = 5;

// The templates are positional, in the shape `std::vformat` reads: `{N}` names
// the argument at that index, and `{{` or `}}` stand for a literal brace
function renderMessage(format, args) {
  if (format === undefined || format === '') return UNKNOWN;

  let result = '';
  let cursor = 0;
  while (cursor < format.length) {
    const character = format[cursor];
    if (character === '}') {
      result += '}';
      cursor += 2;
      continue;
    }

    if (character !== '{') {
      result += character;
      cursor += 1;
      continue;
    }

    if (format[cursor + 1] === '{') {
      result += '{';
      cursor += 2;
      continue;
    }

    let end = cursor + 1;
    while (end < format.length && format[end] >= '0' && format[end] <= '9') {
      end += 1;
    }

    if (end === cursor + 1) {
      result += '{';
      cursor += 1;
      continue;
    }

    const argument = args[Number(format.slice(cursor + 1, end))];
    result += argument === undefined ? '' : argument;
    cursor = end + 1;
  }

  return result;
}

// Whether a template draws on any argument at all, which is how the evaluator
// tells the forms that state something of the instance from the ones that
// state nothing but the schema
function takesArguments(format) {
  return /\{[0-9]/.test(format);
}

function typeName(typeIndex) {
  return TYPE_NAMES[typeIndex];
}

function jsonTypeOf(value) {
  if (value === null) return 0;
  switch (typeof value) {
    case 'boolean': return 1;
    case 'number': return Number.isInteger(value) ? 2 : 3;
    case 'bigint': return 2;
    case 'string': return 4;
    case 'object': return Array.isArray(value) ? 5 : 6;
    default: return 0;
  }
}

function valueTypeName(value) {
  if (typeof value === 'bigint') return 'integer';
  if (typeof value === 'number') {
    return Number.isInteger(value) ? 'integer' : 'number';
  }
  return typeName(jsonTypeOf(value));
}

function escapeString(input) {
  return '"' + String(input).replaceAll('"', '\\"') + '"';
}

function stringifyValue(value) {
  if (typeof value === 'bigint') return String(value);
  return JSON.stringify(value, (key, current) =>
    typeof current === 'bigint' ? JSON.rawJSON(String(current)) : current);
}

function resolveTarget(instance, instanceLocation) {
  if (instanceLocation === '') return instance;
  const tokens = instanceLocation.slice(1).split('/');
  let current = instance;
  for (const raw of tokens) {
    const token = raw.replaceAll('~1', '/').replaceAll('~0', '~');
    if (Array.isArray(current)) {
      current = current[Number(token)];
    } else {
      current = current[token];
    }
  }
  return current;
}

function instanceParent(instance, instanceLocation) {
  if (instanceLocation === '') return undefined;
  const lastSlash = instanceLocation.lastIndexOf('/');
  return resolveTarget(instance, instanceLocation.slice(0, lastSlash));
}

function extractKeyword(evaluatePath) {
  if (evaluatePath === '') return '';
  const lastSlash = evaluatePath.lastIndexOf('/');
  if (lastSlash === -1) return '';
  const token = evaluatePath.slice(lastSlash + 1)
    .replaceAll('~1', '/').replaceAll('~0', '~');
  if (/^[0-9]+$/.test(token)) return '';
  return token;
}

function isWithinKeyword(evaluatePath, keyword) {
  const segments = evaluatePath.split('/');
  for (let index = 1; index < segments.length; index++) {
    if (segments[index].replaceAll('~1', '/').replaceAll('~0', '~') === keyword) {
      return true;
    }
  }
  return false;
}

function lastInstanceToken(instanceLocation) {
  if (instanceLocation === '') return null;
  const lastSlash = instanceLocation.lastIndexOf('/');
  return instanceLocation.slice(lastSlash + 1)
    .replaceAll('~1', '/').replaceAll('~0', '~');
}

// Inside `propertyNames` the subject of a message is the property name that
// the evaluator carries out of band rather than the instance the location
// points at. A property the author happened to name `propertyNames` reads the
// same way in the evaluation path, so the instance location has to agree:
// only a location that names a property of an object can be one
function namesAProperty(evaluatePath, instanceLocation, instance) {
  return isWithinKeyword(evaluatePath, 'propertyNames') &&
    instanceLocation !== '' &&
    isObject(instanceParent(instance, instanceLocation));
}

function objectSize(value) {
  let count = 0;
  for (const _key in value) count++;
  return count;
}

function objectKeys(value) {
  const keys = [];
  for (const key in value) keys.push(key);
  return keys;
}

function unicodeLength(string) {
  let count = 0;
  for (let index = 0; index < string.length; index++) {
    count++;
    const code = string.charCodeAt(index);
    if (code >= 0xD800 && code <= 0xDBFF) index++;
  }
  return count;
}

function isObject(value) {
  return value !== null && typeof value === 'object' && !Array.isArray(value);
}

function jsonEqual(left, right) {
  if (left === right) return true;
  if (left === null || right === null) return left === right;
  if (typeof left !== typeof right) return false;
  if (typeof left !== 'object') return left === right;
  if (Array.isArray(left) !== Array.isArray(right)) return false;
  if (Array.isArray(left)) {
    if (left.length !== right.length) return false;
    for (let index = 0; index < left.length; index++) {
      if (!jsonEqual(left[index], right[index])) return false;
    }
    return true;
  }
  const leftKeys = Object.keys(left).sort();
  const rightKeys = Object.keys(right).sort();
  if (leftKeys.length !== rightKeys.length) return false;
  for (let index = 0; index < leftKeys.length; index++) {
    if (leftKeys[index] !== rightKeys[index]) return false;
    if (!jsonEqual(left[leftKeys[index]], right[rightKeys[index]])) return false;
  }
  return true;
}

function jsonCompare(left, right) {
  const leftType = jsonTypeOf(left);
  const rightType = jsonTypeOf(right);
  if (leftType !== rightType) return leftType - rightType;
  if (left === null) return 0;
  if (typeof left === 'boolean') return (left ? 1 : 0) - (right ? 1 : 0);
  if (typeof left === 'number' || typeof left === 'bigint') {
    if (left < right) return -1;
    if (left > right) return 1;
    return 0;
  }
  if (typeof left === 'string') return left < right ? -1 : left > right ? 1 : 0;
  const leftStr = JSON.stringify(left);
  const rightStr = JSON.stringify(right);
  return leftStr < rightStr ? -1 : leftStr > rightStr ? 1 : 0;
}

function normalizeTypes(bitmask) {
  let types = bitmask;
  const hasReal = (types & (1 << TYPE_REAL)) !== 0;
  const hasInteger = (types & (1 << TYPE_INTEGER)) !== 0;
  const hasDecimal = (types & (1 << 7)) !== 0;
  if (hasReal && hasInteger) types &= ~(1 << TYPE_INTEGER);
  if (hasReal && hasDecimal) types &= ~(1 << 7);
  if (hasInteger && hasDecimal) types &= ~(1 << 7);
  return types;
}

function typeBits(bitmask) {
  const bits = [];
  for (let bit = 0; bit < 8; bit++) {
    if ((bitmask & (1 << bit)) !== 0) bits.push(bit);
  }
  return bits;
}

// The types a set names, as a single type or as a list of alternatives
function describeTypes(bitmask) {
  const bits = typeBits(normalizeTypes(bitmask));
  if (bits.length === 1) return typeName(bits[0]);

  let result = '';
  for (let index = 0; index < bits.length; index++) {
    if (index > 0) result += ', ';
    if (index === bits.length - 1) result += 'or ';
    result += typeName(bits[index]);
  }
  return result;
}

// Which numeric type to report for the instance, given which numeric types the
// message named as alternatives. A set that narrowed to one of them reports
// that one, as the broader name would read as a contradiction
function describeActualType(narrowed, currentType, hasReal) {
  if (narrowed && currentType === TYPE_INTEGER && hasReal) return 'number';
  return typeName(currentType);
}

function describeTypeCheck(instruction, subject, valid, currentType,
                           expectedType) {
  if (valid) {
    return renderMessage(instruction.message,
                         [ subject, typeName(expectedType) ]);
  }

  return renderMessage(instruction.messageDetail,
                       [ subject, typeName(expectedType),
                         typeName(currentType) ]);
}

function describeNotTypeCheck(instruction, subject, valid, currentType,
                              expectedType) {
  if (valid) {
    return renderMessage(instruction.message,
                         [ subject, typeName(expectedType) ]);
  }

  const actual = currentType === TYPE_INTEGER && expectedType === TYPE_REAL
    ? 'number'
    : typeName(currentType);
  return renderMessage(instruction.messageDetail,
                       [ subject, typeName(expectedType), actual ]);
}

function describeTypesCheck(instruction, subject, valid, currentType,
                            bitmask) {
  const types = normalizeTypes(bitmask);
  const bits = typeBits(types);
  if (bits.length === 1) {
    return describeTypeCheck(instruction, subject, valid, currentType, bits[0]);
  }

  const hasReal = (bitmask & (1 << TYPE_REAL)) !== 0;
  return renderMessage(resultFormat(instruction, valid),
                       [ subject, describeTypes(bitmask),
                         describeActualType(valid, currentType, hasReal) ]);
}

function describeNotTypesCheck(instruction, subject, valid, currentType,
                               bitmask) {
  const types = normalizeTypes(bitmask);
  const bits = typeBits(types);
  if (bits.length === 1) {
    return describeNotTypeCheck(instruction, subject, valid, currentType,
                                bits[0]);
  }

  const hasReal = (bitmask & (1 << TYPE_REAL)) !== 0;
  return renderMessage(resultFormat(instruction, valid),
                       [ subject, describeTypes(bitmask),
                         describeActualType(!valid, currentType, hasReal) ]);
}

function formatList(items, conjunction) {
  if (items.length === 1) {
    return items[0];
  }

  let result = '';
  for (let index = 0; index < items.length; index++) {
    if (index === items.length - 1) {
      result += conjunction + ' ' + items[index];
    } else {
      result += items[index] + ', ';
    }
  }
  return result;
}

// The noun phrase a message opens with
function describeSubject(property, instanceLocation, target, withValue) {
  if (property) {
    return 'The property name ' +
      escapeString(lastInstanceToken(instanceLocation));
  }

  return 'The ' + typeName(jsonTypeOf(target)) + ' value' +
    (withValue ? ' ' + stringifyValue(target) : '');
}

// As above, for a message about a string, where the instance is quoted rather
// than serialized
function describeStringSubject(property, instanceLocation, target) {
  if (property) {
    return 'The property name ' +
      escapeString(lastInstanceToken(instanceLocation));
  }

  return 'The string value ' + escapeString(target);
}

// The scope an annotation applies to, as the instance itself or the location
// within it
function describeScope(instanceLocation) {
  if (instanceLocation === '') return 'instance';
  return 'instance location "' + instanceLocation + '"';
}

// Which of the two slots an instruction renders is what says whether the
// instance met its expectation, so nothing in the sentence has to
function resultFormat(instruction, valid) {
  return valid ? instruction.message : instruction.messageDetail;
}

function describeStringSize(instruction, valid, property, instanceLocation,
                            target) {
  const size = property
    ? unicodeLength(lastInstanceToken(instanceLocation))
    : unicodeLength(target);
  return renderMessage(resultFormat(instruction, valid),
                       [ describeSubject(property, instanceLocation,
                                         target, true), size ]);
}

function describeArraySize(instruction, valid, target) {
  return renderMessage(resultFormat(instruction, valid), [ target.length ]);
}

// As above, except that the message also names the properties the instance
// turned out to carry, which an empty object has none of
function describeObjectSize(instruction, valid, target) {
  const size = objectSize(target);
  const carried = size === 0
    ? ''
    : ': ' + formatList(objectKeys(target).sort().map(escapeString), 'and');
  return renderMessage(resultFormat(instruction, valid), [ size, carried ]);
}

// The properties the object defines beyond the ones the set names
function unexpectedProperties(target, allowed) {
  const extras = [];
  for (const key in target) {
    if (!allowed.has(key)) extras.push(key);
  }
  extras.sort();
  return extras;
}

function describeComparison(instruction, target, value) {
  return renderMessage(instruction.message,
                       [ valueTypeName(target), stringifyValue(target),
                         valueTypeName(value), stringifyValue(value) ]);
}

// The sentence an instruction renders when the compiler resolved every word
// of it
function describeLiteral(instruction) {
  const format = instruction.message;
  return format === undefined || format === '' ? UNKNOWN : format;
}

// The opcodes whose sentence the compiler resolved every word of, as what they
// say does not depend on how the instance turned out
const LITERAL_OPCODES = new Set([
  EVALUATE, LOOP_PROPERTIES, LOOP_PROPERTIES_EVALUATE,
  LOOP_PROPERTIES_UNEVALUATED, LOOP_PROPERTIES_UNEVALUATED_EXCEPT,
  LOOP_PROPERTIES_EXCEPT, ASSERTION_OBJECT_PROPERTIES_SIMPLE,
  LOOP_ITEMS, LOOP_ITEMS_FROM, LOOP_ITEMS_UNEVALUATED,
  LOOP_ITEMS_INTEGER_BOUNDED, LOOP_ITEMS_INTEGER_BOUNDED_SIZED,
  LOOP_ITEMS_ARRAY_BOUNDED_TYPE_STRICT_ANY, LOOP_CONTAINS,
  ASSERTION_TYPE_INTEGER_BOUNDED, ASSERTION_TYPE_INTEGER_BOUNDED_STRICT,
  ASSERTION_TYPE_INTEGER_LOWER_BOUND, ASSERTION_TYPE_INTEGER_LOWER_BOUND_STRICT,
  ASSERTION_TYPE_STRING_BOUNDED, ASSERTION_TYPE_STRING_UPPER,
  ASSERTION_TYPE_ARRAY_BOUNDED, ASSERTION_TYPE_ARRAY_UPPER,
  ASSERTION_TYPE_OBJECT_BOUNDED, ASSERTION_TYPE_OBJECT_UPPER
]);

export function describe(valid, instruction, evaluatePath,
                         instanceLocation, instance, annotation) {
  const opcode = instruction[0];
  const value = instruction[6];
  const children = instruction[7];
  const keyword = extractKeyword(evaluatePath);
  const target = resolveTarget(instance, instanceLocation);
  const targetType = jsonTypeOf(target);
  const property = namesAProperty(evaluatePath, instanceLocation, instance);

  if (LITERAL_OPCODES.has(opcode)) {
    return describeLiteral(instruction);
  }

  if (opcode === ASSERTION_FAIL) {
    // A `false` subschema declares no keyword of its own, so its evaluation
    // path ends at whatever declared it, which under `properties` and its
    // relatives is a name the author chose. A name that happens to read like
    // one of the keywords that name what the instance carried must not be
    // taken for the keyword itself, so the instance location has to agree
    const detail = instruction.messageDetail;
    if (detail !== undefined && detail !== '' && instanceLocation !== '') {
      const parent = instanceParent(instance, instanceLocation);
      if (keyword === 'unevaluatedItems') {
        if (Array.isArray(parent)) {
          return renderMessage(detail, [ lastInstanceToken(instanceLocation) ]);
        }
      } else if (isObject(parent)) {
        return renderMessage(detail,
                             [ escapeString(lastInstanceToken(instanceLocation)) ]);
      }
    }

    return renderMessage(instruction.message, [ typeName(targetType) ]);
  }

  if (opcode === LOGICAL_OR || opcode === LOGICAL_AND ||
      opcode === LOGICAL_CONDITION || opcode === LOGICAL_NOT ||
      opcode === LOGICAL_NOT_EVALUATE) {
    return renderMessage(instruction.message, [ typeName(targetType) ]);
  }

  if (opcode === LOGICAL_XOR) {
    return renderMessage(instruction.message,
                         [ describeSubject(property, instanceLocation,
                                           target, false) ]);
  }

  if (opcode === CONTROL_JUMP) {
    // Inside `propertyNames` the subject is the property name, which is a
    // string rather than the instance the location points at
    return renderMessage(instruction.message,
                         [ property ? 'string' : typeName(targetType) ]);
  }

  if (opcode === CONTROL_DYNAMIC_ANCHOR_JUMP) {
    return renderMessage(instruction.message,
                         [ typeName(targetType), escapeString(value) ]);
  }

  if (opcode === ANNOTATION_EMIT || opcode === ANNOTATION_EMIT_WRAPPED ||
      opcode === ANNOTATION_TO_PARENT) {
    const args = [];
    args[ANNOTATION_SCOPE] = describeScope(instanceLocation);
    args[ANNOTATION_QUOTED] = typeof annotation === 'string'
      ? escapeString(annotation) : '';
    args[ANNOTATION_SPELLED] = stringifyValue(annotation);
    args[ANNOTATION_LIST] = Array.isArray(annotation)
      ? formatList(annotation.map(stringifyValue), 'and') : '';
    args[ANNOTATION_KEYWORD] = escapeString(keyword);
    args[ANNOTATION_JSON] = stringifyValue(annotation);
    return renderMessage(instruction.message, args);
  }

  if (opcode === ANNOTATION_BASENAME_TO_PARENT) {
    return renderMessage(instruction.message,
                         [ typeof annotation === 'string'
                             ? escapeString(annotation)
                             : stringifyValue(annotation) ]);
  }

  if (opcode === LOOP_PROPERTIES_EXACTLY_TYPE_STRICT ||
      opcode === LOOP_PROPERTIES_EXACTLY_TYPE_STRICT_HASH ||
      opcode === LOOP_ITEMS_PROPERTIES_EXACTLY_TYPE_STRICT_HASH ||
      opcode === LOOP_ITEMS_PROPERTIES_EXACTLY_TYPE_STRICT_HASH3) {
    return renderMessage(instruction.message, [ typeName(value[0]) ]);
  }

  if (opcode === ASSERTION_NUMBER_TYPE ||
      opcode === ASSERTION_NUMBER_INTEGER_BOUNDED) {
    return renderMessage(instruction.message,
                         [ valueTypeName(target), stringifyValue(target) ]);
  }

  if (opcode === LOOP_PROPERTIES_TYPE ||
      opcode === LOOP_PROPERTIES_TYPE_EVALUATE ||
      opcode === LOOP_PROPERTIES_TYPE_STRICT ||
      opcode === LOOP_PROPERTIES_TYPE_STRICT_EVALUATE ||
      opcode === LOOP_ITEMS_TYPE || opcode === LOOP_ITEMS_TYPE_STRICT) {
    return renderMessage(instruction.message, [ typeName(value) ]);
  }

  if (opcode === LOOP_PROPERTIES_TYPE_STRICT_ANY ||
      opcode === LOOP_PROPERTIES_TYPE_STRICT_ANY_EVALUATE ||
      opcode === LOOP_ITEMS_TYPE_STRICT_ANY) {
    return renderMessage(instruction.message, [ describeTypes(value) ]);
  }

  if (opcode === LOOP_KEYS) {
    const keys = objectKeys(target);
    if (keys.length === 0) {
      return describeLiteral(instruction);
    }

    return renderMessage(instruction.messageDetail,
                         [ formatList(keys.map(escapeString), 'and') ]);
  }

  if (opcode === ASSERTION_DEFINES || opcode === ASSERTION_DEFINES_STRICT) {
    return renderMessage(instruction.message, [ escapeString(value) ]);
  }

  if (opcode === ASSERTION_DEFINES_ALL) {
    const listed = formatList(value.map(escapeString), 'and');
    if (valid) {
      return renderMessage(instruction.message, [ listed ]);
    }

    const missing = [];
    for (let index = 0; index < value.length; index++) {
      if (!Object.hasOwn(target, value[index])) missing.push(value[index]);
    }
    missing.sort();

    return renderMessage(instruction.messageDetail,
                         [ listed, formatList(missing.map(escapeString), 'and') ]);
  }

  if (opcode === ASSERTION_DEFINES_ALL_STRICT) {
    return renderMessage(instruction.message,
                         [ formatList(value.map(escapeString), 'and') ]);
  }

  if (opcode === ASSERTION_DEFINES_EXACTLY ||
      opcode === ASSERTION_DEFINES_EXACTLY_STRICT) {
    const listed = formatList([ ...value ].sort().map(escapeString), 'and');
    if (valid || !isObject(target)) {
      return renderMessage(instruction.message, [ listed ]);
    }

    const extras = unexpectedProperties(target, new Set(value));
    if (extras.length === 0) {
      return renderMessage(instruction.message, [ listed ]);
    }

    return renderMessage(instruction.messageDetail,
                         [ listed,
                           formatList(extras.map(escapeString), 'and') ]);
  }

  if (opcode === ASSERTION_DEFINES_EXACTLY_STRICT_HASH3) {
    if (valid || !isObject(target)) {
      return describeLiteral(instruction);
    }

    const allowed = new Set(value[0].map(entry => entry[1]));
    const extras = unexpectedProperties(target, allowed);
    if (extras.length === 0) {
      return describeLiteral(instruction);
    }

    return renderMessage(instruction.messageDetail,
                         [ formatList(extras.map(escapeString), 'and') ]);
  }

  if (opcode === ASSERTION_TYPE || opcode === ASSERTION_TYPE_STRICT) {
    // Inside `propertyNames` the subject is the property name, which is a
    // string however the assertion turns out, so there is nothing to report
    // beyond the type the keyword asked for
    const subject = property
      ? 'The property name ' + escapeString(lastInstanceToken(instanceLocation))
      : 'The value';
    return describeTypeCheck(instruction, subject, valid || property,
                             targetType, value);
  }

  if (opcode === ASSERTION_TYPE_ANY || opcode === ASSERTION_TYPE_STRICT_ANY ||
      opcode === ASSERTION_PROPERTY_TYPE_STRICT_ANY ||
      opcode === ASSERTION_PROPERTY_TYPE_STRICT_ANY_EVALUATE) {
    return describeTypesCheck(instruction, 'The value', valid, targetType,
                              value);
  }

  if (opcode === ASSERTION_PROPERTY_TYPE ||
      opcode === ASSERTION_PROPERTY_TYPE_EVALUATE ||
      opcode === ASSERTION_PROPERTY_TYPE_STRICT ||
      opcode === ASSERTION_PROPERTY_TYPE_STRICT_EVALUATE) {
    return describeTypeCheck(instruction, 'The value', valid, targetType,
                             value);
  }

  if (opcode === ASSERTION_NOT_TYPE_STRICT_ANY) {
    // The keywords whose bound no instance can satisfy compile into a type
    // check that explains the keyword instead, in one sentence of its own
    if (!takesArguments(instruction.message)) {
      return describeLiteral(instruction);
    }

    return describeNotTypesCheck(instruction, 'The value', valid, targetType,
                                 value);
  }

  if (opcode === ASSERTION_REGEX) {
    return renderMessage(instruction.message,
                         [ describeStringSubject(property, instanceLocation,
                                                 target),
                           escapeString(value.source) ]);
  }

  if (opcode === ASSERTION_STRING_SIZE_LESS ||
      opcode === ASSERTION_STRING_SIZE_GREATER) {
    return describeStringSize(instruction, valid, property, instanceLocation,
                              target);
  }

  if (opcode === ASSERTION_ARRAY_SIZE_LESS ||
      opcode === ASSERTION_ARRAY_SIZE_GREATER) {
    return describeArraySize(instruction, valid, target);
  }

  if (opcode === ASSERTION_OBJECT_SIZE_LESS) {
    // A closed object says so without counting, as the bound it compiled from
    // is the number of properties the schema named
    if (!takesArguments(instruction.message)) {
      return describeLiteral(instruction);
    }

    return describeObjectSize(instruction, valid, target);
  }

  if (opcode === ASSERTION_OBJECT_SIZE_GREATER) {
    return describeObjectSize(instruction, valid, target);
  }

  if (opcode === ASSERTION_PROPERTY_EQUAL) {
    return renderMessage(instruction.message,
                         [ typeName(targetType), stringifyValue(target),
                           typeName(jsonTypeOf(value)),
                           stringifyValue(value) ]);
  }

  if (opcode === ASSERTION_EQUAL) {
    return renderMessage(instruction.message,
                         [ describeSubject(property, instanceLocation,
                                           target, true),
                           typeName(jsonTypeOf(value)),
                           stringifyValue(value) ]);
  }

  if (opcode === ASSERTION_GREATER_EQUAL || opcode === ASSERTION_LESS_EQUAL ||
      opcode === ASSERTION_GREATER || opcode === ASSERTION_LESS ||
      opcode === ASSERTION_DIVISIBLE) {
    return describeComparison(instruction, target, value);
  }

  if (opcode === ASSERTION_UNIQUE) {
    if (valid) {
      return describeLiteral(instruction);
    }

    const duplicates = new Set();
    for (let index = 0; index < target.length; index++) {
      for (let other = index + 1; other < target.length; other++) {
        if (jsonEqual(target[index], target[other])) {
          duplicates.add(stringifyValue(target[index]));
        }
      }
    }

    return renderMessage(instruction.messageDetail,
                         [ formatList([ ...duplicates ].sort(), 'and') ]);
  }

  if (opcode === ASSERTION_EQUALS_ANY) {
    const subject = describeSubject(property, instanceLocation, target,
                                    true);
    const values = Array.isArray(value) ? value : (value.values || []);

    if (values.length === 1) {
      return renderMessage(instruction.message,
                           [ subject, typeName(jsonTypeOf(values[0])),
                             stringifyValue(values[0]) ]);
    }

    if (valid) {
      return renderMessage(instruction.message, [ subject ]);
    }

    const sorted = [ ...values ].sort(jsonCompare);
    return renderMessage(instruction.messageDetail,
                         [ subject,
                           formatList(sorted.map(stringifyValue), 'and') ]);
  }

  if (opcode === ASSERTION_EQUALS_ANY_STRING_HASH) {
    return renderMessage(instruction.message,
                         [ describeSubject(property, instanceLocation,
                                           target, true) ]);
  }

  if (opcode === ASSERTION_STRING_TYPE) {
    return renderMessage(instruction.message, [ escapeString(target) ]);
  }

  if (opcode === ASSERTION_ARRAY_PREFIX ||
      opcode === ASSERTION_ARRAY_PREFIX_EVALUATE) {
    return renderMessage(instruction.message, [ children.length - 1 ]);
  }

  if (opcode === LOOP_PROPERTIES_MATCH ||
      opcode === LOOP_PROPERTIES_MATCH_CLOSED) {
    return renderMessage(instruction.message, [ children.length ]);
  }

  if (opcode === LOGICAL_WHEN_DEFINES) {
    return renderMessage(instruction.message, [ value ]);
  }

  if (opcode === LOOP_PROPERTIES_REGEX ||
      opcode === LOOP_PROPERTIES_REGEX_CLOSED) {
    return renderMessage(instruction.message, [ value.source ]);
  }

  if (opcode === LOOP_PROPERTIES_STARTS_WITH) {
    return renderMessage(instruction.message, [ value ]);
  }

  if (opcode === LOGICAL_WHEN_TYPE) {
    if (keyword === 'items') {
      return describeTypeCheck(instruction, 'The value', valid, targetType,
                               value);
    }

    // The evaluator only reaches this instruction when the instance is of the
    // type it names, so there is never a mismatch of its own to report
    if (keyword === 'properties') {
      return describeLiteral(instruction);
    }

    if (keyword === 'dependentSchemas') {
      const present = new Set();
      const allDependencies = new Set();
      for (let index = 0; index < children.length; index++) {
        const property = children[index][6];
        allDependencies.add(property);
        if (Object.hasOwn(target, property)) present.add(property);
      }

      if (present.size === 0) {
        return renderMessage(
          instruction.message,
          [ formatList([ ...allDependencies ].sort().map(escapeString),
                       'or') ]);
      }

      return renderMessage(
        instruction.messageDetail,
        [ formatList([ ...present ].sort().map(escapeString), 'and') ]);
    }

    return UNKNOWN;
  }

  if (opcode === ASSERTION_PROPERTY_DEPENDENCIES) {
    const present = [];
    const allDependencies = [];
    const required = new Set();

    for (const property in value) {
      allDependencies.push(property);
      if (!Object.hasOwn(target, property)) continue;
      present.push(property);
      for (const dependency of value[property]) {
        if (valid || !Object.hasOwn(target, dependency)) {
          required.add(dependency);
        }
      }
    }

    if (present.length === 0) {
      return renderMessage(
        instruction.message,
        [ formatList(allDependencies.sort().map(escapeString), 'or') ]);
    }

    return renderMessage(
      instruction.messageDetail,
      [ formatList(present.sort().map(escapeString), 'and'),
        formatList([ ...required ].sort().map(escapeString), 'and') ]);
  }

  if (opcode === LOGICAL_WHEN_ARRAY_SIZE_GREATER) {
    if (target.length <= value) {
      return describeLiteral(instruction);
    }

    return instruction.messageDetail;
  }

  return UNKNOWN;
}
