import {
  ASSERTION_FAIL, ASSERTION_DEFINES, ASSERTION_DEFINES_STRICT,
  ASSERTION_DEFINES_ALL, ASSERTION_DEFINES_ALL_STRICT,
  ASSERTION_DEFINES_EXACTLY, ASSERTION_DEFINES_EXACTLY_STRICT,
  ASSERTION_DEFINES_EXACTLY_STRICT_HASH3, ASSERTION_PROPERTY_DEPENDENCIES,
  ASSERTION_TYPE, ASSERTION_TYPE_ANY, ASSERTION_TYPE_STRICT,
  ASSERTION_TYPE_STRICT_ANY, ASSERTION_TYPE_STRING_BOUNDED,
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
  ASSERTION_STRING_TYPE,
  ASSERTION_PROPERTY_TYPE, ASSERTION_PROPERTY_TYPE_EVALUATE,
  ASSERTION_PROPERTY_TYPE_STRICT, ASSERTION_PROPERTY_TYPE_STRICT_EVALUATE,
  ASSERTION_PROPERTY_TYPE_STRICT_ANY, ASSERTION_PROPERTY_TYPE_STRICT_ANY_EVALUATE,
  ASSERTION_ARRAY_PREFIX, ASSERTION_ARRAY_PREFIX_EVALUATE,
  ASSERTION_OBJECT_PROPERTIES_SIMPLE,
  ANNOTATION_EMIT, ANNOTATION_TO_PARENT, ANNOTATION_BASENAME_TO_PARENT,
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
  LOOP_CONTAINS,
  CONTROL_DYNAMIC_ANCHOR_JUMP, CONTROL_JUMP
} from './opcodes.mjs';

const TYPE_INTEGER = 2;
const TYPE_REAL = 3;
const TYPE_OBJECT = 6;

const TYPE_NAMES = [ 'null', 'boolean', 'integer', 'number',
                     'string', 'array', 'object', 'number' ];

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

function describeTypeCheck(valid, currentType, expectedType) {
  let message = 'The value was expected to be of type ' + typeName(expectedType);
  if (!valid) {
    message += ' but it was of type ' + typeName(currentType);
  }
  return message;
}

function describeTypesCheck(valid, currentType, bitmask) {
  let types = normalizeTypes(bitmask);
  const hasReal = (bitmask & (1 << TYPE_REAL)) !== 0;
  const hasInteger = (bitmask & (1 << TYPE_INTEGER)) !== 0;

  let popcount = 0;
  for (let bit = 0; bit < 8; bit++) {
    if ((types & (1 << bit)) !== 0) popcount++;
  }

  if (popcount === 1) {
    let typeIndex = 0;
    for (let bit = 0; bit < 8; bit++) {
      if ((types & (1 << bit)) !== 0) { typeIndex = bit; break; }
    }
    return describeTypeCheck(valid, currentType, typeIndex);
  }

  let message = 'The value was expected to be of type ';
  let first = true;
  let lastBit = 0;
  for (let bit = 0; bit < 8; bit++) {
    if ((types & (1 << bit)) !== 0) lastBit = bit;
  }
  for (let bit = 0; bit < 8; bit++) {
    if ((types & (1 << bit)) !== 0) {
      if (!first) message += ', ';
      if (bit === lastBit) message += 'or ';
      message += typeName(bit);
      first = false;
    }
  }

  if (valid) {
    message += ' and it was of type ';
  } else {
    message += ' but it was of type ';
  }

  if (valid && currentType === TYPE_INTEGER && hasReal) {
    message += 'number';
  } else if ((valid && currentType === TYPE_INTEGER && hasReal) ||
             currentType === TYPE_REAL) {
    message += 'number';
  } else {
    message += typeName(currentType);
  }

  return message;
}

function describeReference(target) {
  return 'The ' + typeName(jsonTypeOf(target)) +
    ' value was expected to validate against the referenced schema';
}

function describeTypeList(bitmask) {
  let types = normalizeTypes(bitmask);

  let popcount = 0;
  for (let bit = 0; bit < 8; bit++) {
    if ((types & (1 << bit)) !== 0) popcount++;
  }

  if (popcount === 1) {
    for (let bit = 0; bit < 8; bit++) {
      if ((types & (1 << bit)) !== 0) return typeName(bit);
    }
  }

  let result = '';
  let first = true;
  let lastBit = 0;
  for (let bit = 0; bit < 8; bit++) {
    if ((types & (1 << bit)) !== 0) lastBit = bit;
  }
  for (let bit = 0; bit < 8; bit++) {
    if ((types & (1 << bit)) !== 0) {
      if (!first) result += ', ';
      if (bit === lastBit) result += 'or ';
      result += typeName(bit);
      first = false;
    }
  }
  return result;
}

function formatList(items, conjunction) {
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

export function describe(valid, instruction, evaluatePath,
                         instanceLocation, instance, annotation) {
  const opcode = instruction[0];
  const value = instruction[5];
  const children = instruction[6];
  const keyword = extractKeyword(evaluatePath);
  const target = resolveTarget(instance, instanceLocation);
  const targetType = jsonTypeOf(target);

  if (opcode === ASSERTION_FAIL) {
    if (keyword === 'enum') {
      return 'The ' + typeName(targetType) +
        ' value was not expected to validate against the empty enumeration';
    }
    if (keyword === 'contains') {
      return 'The constraints declared for this keyword were not satisfiable';
    }
    if (keyword === 'additionalProperties' ||
        keyword === 'unevaluatedProperties') {
      const property = lastInstanceToken(instanceLocation);
      return 'The object value was not expected to define the property ' +
        escapeString(property);
    }
    if (keyword === 'unevaluatedItems') {
      const tokenValue = lastInstanceToken(instanceLocation);
      return 'The array value was not expected to define the item at index ' +
        tokenValue;
    }
    return 'No instance is expected to succeed against the false schema';
  }

  if (opcode === LOGICAL_OR) {
    const childCount = children ? children.length : 0;
    let message = 'The ' + typeName(targetType) +
      ' value was expected to validate against ';
    if (childCount > 1) {
      message += 'at least one of the ' + childCount + ' given subschemas';
    } else {
      message += 'the given subschema';
    }
    return message;
  }

  if (opcode === LOGICAL_AND) {
    if (keyword === 'allOf') {
      const childCount = children ? children.length : 0;
      let message = 'The ' + typeName(targetType) +
        ' value was expected to validate against the ';
      if (childCount > 1) {
        message += childCount + ' given subschemas';
      } else {
        message += 'given subschema';
      }
      return message;
    }
    if (keyword === '$ref') {
      return describeReference(target);
    }
    return '<unknown>';
  }

  if (opcode === LOGICAL_XOR) {
    const childCount = children ? children.length : 0;
    let message = '';
    if (isWithinKeyword(evaluatePath, 'propertyNames') &&
        instanceLocation !== '' && lastInstanceToken(instanceLocation) !== null) {
      const propertyName = lastInstanceToken(instanceLocation);
      message += 'The property name ' + escapeString(propertyName);
    } else {
      message += 'The ' + typeName(targetType) + ' value';
    }
    message += ' was expected to validate against ';
    if (childCount > 1) {
      message += 'one and only one of the ' + childCount + ' given subschemas';
    } else {
      message += 'the given subschema';
    }
    return message;
  }

  if (opcode === LOGICAL_CONDITION) {
    return 'The ' + typeName(targetType) +
      ' value was expected to validate against the given conditional';
  }

  if (opcode === LOGICAL_NOT) {
    let message = 'The ' + typeName(targetType) +
      ' value was expected to not validate against the given subschema';
    if (!valid) message += ', but it did';
    return message;
  }

  if (opcode === LOGICAL_NOT_EVALUATE) {
    let message = 'The ' + typeName(targetType) +
      ' value was expected to not validate against the given subschema';
    if (!valid) message += ', but it did';
    return message;
  }

  if (opcode === EVALUATE) {
    return 'The instance location was marked as evaluated';
  }

  if (opcode === CONTROL_DYNAMIC_ANCHOR_JUMP) {
    if (keyword === '$dynamicRef') {
      return 'The ' + typeName(targetType) +
        ' value was expected to validate against the first subschema ' +
        'in scope that declared the dynamic anchor ' + escapeString(value);
    }
    return 'The ' + typeName(targetType) +
      ' value was expected to validate against the first subschema ' +
      'in scope that declared a recursive anchor';
  }

  if (opcode === ANNOTATION_EMIT) {
    if (keyword === 'properties') {
      return 'The object property ' + escapeString(annotation) +
        ' successfully validated against its property subschema';
    }

    if ((keyword === 'items' || keyword === 'additionalItems') &&
        annotation === true) {
      return 'Every item in the array value was successfully validated';
    }

    if ((keyword === 'prefixItems' || keyword === 'items') &&
        typeof annotation === 'number') {
      if (annotation === 0) {
        return 'The first item of the array value successfully validated ' +
          'against the first positional subschema';
      }
      return 'The first ' + (annotation + 1) +
        ' items of the array value successfully validated against the given ' +
        'positional subschemas';
    }

    if (keyword === 'prefixItems' && annotation === true) {
      return 'Every item of the array value validated against the given ' +
        'positional subschemas';
    }

    if (keyword === 'title' || keyword === 'description') {
      let message = 'The ' + keyword + ' of the';
      if (instanceLocation === '') {
        message += ' instance';
      } else {
        message += ' instance location "' + instanceLocation + '"';
      }
      message += ' was ' + escapeString(annotation);
      return message;
    }

    if (keyword === 'default') {
      let message = 'The default value of the';
      if (instanceLocation === '') {
        message += ' instance';
      } else {
        message += ' instance location "' + instanceLocation + '"';
      }
      message += ' was ' + stringifyValue(annotation);
      return message;
    }

    if (keyword === 'deprecated' && typeof annotation === 'boolean') {
      let message = '';
      if (instanceLocation === '') {
        message += 'The instance';
      } else {
        message += 'The instance location "' + instanceLocation + '"';
      }
      message += annotation
        ? ' was considered deprecated'
        : ' was not considered deprecated';
      return message;
    }

    if (keyword === 'readOnly' && typeof annotation === 'boolean') {
      let message = '';
      if (instanceLocation === '') {
        message += 'The instance';
      } else {
        message += 'The instance location "' + instanceLocation + '"';
      }
      message += annotation
        ? ' was considered read-only'
        : ' was not considered read-only';
      return message;
    }

    if (keyword === 'writeOnly' && typeof annotation === 'boolean') {
      let message = '';
      if (instanceLocation === '') {
        message += 'The instance';
      } else {
        message += 'The instance location "' + instanceLocation + '"';
      }
      message += annotation
        ? ' was considered write-only'
        : ' was not considered write-only';
      return message;
    }

    if (keyword === 'examples') {
      let message = '';
      if (instanceLocation === '') {
        message += 'Examples of the instance';
      } else {
        message += 'Examples of the instance location "' +
          instanceLocation + '"';
      }
      message += ' were ';
      for (let index = 0; index < annotation.length; index++) {
        if (index === annotation.length - 1) {
          message += 'and ' + stringifyValue(annotation[index]);
        } else {
          message += stringifyValue(annotation[index]) + ', ';
        }
      }
      return message;
    }

    if (keyword === 'contentEncoding') {
      let message = 'The content encoding of the';
      if (instanceLocation === '') {
        message += ' instance';
      } else {
        message += ' instance location "' + instanceLocation + '"';
      }
      message += ' was ' + escapeString(annotation);
      return message;
    }

    if (keyword === 'contentMediaType') {
      let message = 'The content media type of the';
      if (instanceLocation === '') {
        message += ' instance';
      } else {
        message += ' instance location "' + instanceLocation + '"';
      }
      message += ' was ' + escapeString(annotation);
      return message;
    }

    if (keyword === 'contentSchema') {
      let message = 'When decoded, the';
      if (instanceLocation === '') {
        message += ' instance';
      } else {
        message += ' instance location "' + instanceLocation + '"';
      }
      message += ' was expected to validate against the schema ' +
        stringifyValue(annotation);
      return message;
    }

    if (keyword === 'format') {
      let message = 'The logical type of the';
      if (instanceLocation === '') {
        message += ' instance';
      } else {
        message += ' instance location "' + instanceLocation + '"';
      }
      message += ' was expected to be ' + stringifyValue(annotation);
      return message;
    }

    return 'The unrecognized keyword ' + escapeString(keyword) +
      ' was collected as the annotation ' + stringifyValue(annotation);
  }

  if (opcode === ANNOTATION_TO_PARENT) {
    if (keyword === 'unevaluatedItems' && annotation === true) {
      return 'At least one item of the array value successfully validated ' +
        'against the subschema for unevaluated items';
    }
    return '<unknown>';
  }

  if (opcode === ANNOTATION_BASENAME_TO_PARENT) {
    if (keyword === 'patternProperties') {
      return 'The object property ' + escapeString(String(annotation)) +
        ' successfully validated against its pattern property subschema';
    }
    if (keyword === 'additionalProperties') {
      return 'The object property ' + escapeString(String(annotation)) +
        ' successfully validated against the additional properties subschema';
    }
    if (keyword === 'unevaluatedProperties') {
      return 'The object property ' + escapeString(String(annotation)) +
        ' successfully validated against the subschema for ' +
        'unevaluated properties';
    }
    if (keyword === 'contains' && typeof annotation === 'number') {
      return 'The item at index ' + annotation +
        ' of the array value successfully validated against the ' +
        'containment check subschema';
    }
    return '<unknown>';
  }

  if (opcode === LOOP_PROPERTIES) {
    const childCount = children ? children.length : 0;
    if (childCount > 0 && children[0][0] === ASSERTION_FAIL) {
      if (keyword === 'unevaluatedProperties') {
        return 'The object value was not expected to define unevaluated properties';
      }
      return 'The object value was not expected to define additional properties';
    }
    if (keyword === 'unevaluatedProperties') {
      return 'The object properties not covered by other object ' +
        'keywords were expected to validate against this subschema';
    }
    return 'The object properties not covered by other adjacent object ' +
      'keywords were expected to validate against this subschema';
  }

  if (opcode === LOOP_PROPERTIES_EVALUATE) {
    const childCount = children ? children.length : 0;
    if (childCount === 1 && children[0][0] === ASSERTION_FAIL) {
      return 'The object value was not expected to define additional properties';
    }
    return 'The object properties not covered by other adjacent object ' +
      'keywords were expected to validate against this subschema';
  }

  if (opcode === LOOP_PROPERTIES_UNEVALUATED) {
    if (keyword === 'unevaluatedProperties') {
      const childCount = children ? children.length : 0;
      if (childCount > 0 && children[0][0] === ASSERTION_FAIL) {
        return 'The object value was not expected to define unevaluated properties';
      }
      return 'The object properties not covered by other object ' +
        'keywords were expected to validate against this subschema';
    }
    return '<unknown>';
  }

  if (opcode === LOOP_PROPERTIES_UNEVALUATED_EXCEPT) {
    if (keyword === 'unevaluatedProperties') {
      const childCount = children ? children.length : 0;
      if (childCount > 0 && children[0][0] === ASSERTION_FAIL) {
        return 'The object value was not expected to define unevaluated properties';
      }
      return 'The object properties not covered by other object ' +
        'keywords were expected to validate against this subschema';
    }
    return '<unknown>';
  }

  if (opcode === LOOP_PROPERTIES_EXCEPT) {
    const childCount = children ? children.length : 0;
    if (childCount > 0 && children[0][0] === ASSERTION_FAIL) {
      if (keyword === 'unevaluatedProperties') {
        return 'The object value was not expected to define unevaluated properties';
      }
      return 'The object value was not expected to define additional properties';
    }
    if (keyword === 'unevaluatedProperties') {
      return 'The object properties not covered by other object ' +
        'keywords were expected to validate against this subschema';
    }
    return 'The object properties not covered by other adjacent object ' +
      'keywords were expected to validate against this subschema';
  }

  if (opcode === LOOP_PROPERTIES_EXACTLY_TYPE_STRICT) {
    return 'The required object properties were expected to be of type ' +
      typeName(value[0]);
  }

  if (opcode === LOOP_PROPERTIES_EXACTLY_TYPE_STRICT_HASH) {
    return 'The required object properties were expected to be of type ' +
      typeName(value[0]);
  }

  if (opcode === LOOP_ITEMS_PROPERTIES_EXACTLY_TYPE_STRICT_HASH ||
      opcode === LOOP_ITEMS_PROPERTIES_EXACTLY_TYPE_STRICT_HASH3) {
    return 'Every item in the array was expected to be an object whose ' +
      'required properties were of type ' + typeName(value[0]);
  }

  if (opcode === LOOP_ITEMS_INTEGER_BOUNDED ||
      opcode === LOOP_ITEMS_INTEGER_BOUNDED_SIZED) {
    return 'Every item in the array was expected to be a number within the given range';
  }

  if (opcode === ASSERTION_TYPE_INTEGER_BOUNDED ||
      opcode === ASSERTION_TYPE_INTEGER_BOUNDED_STRICT) {
    return 'The value was expected to be an integer within the given range';
  }

  if (opcode === ASSERTION_TYPE_INTEGER_LOWER_BOUND ||
      opcode === ASSERTION_TYPE_INTEGER_LOWER_BOUND_STRICT) {
    return 'The value was expected to be an integer above the given minimum';
  }

  if (opcode === ASSERTION_OBJECT_PROPERTIES_SIMPLE) {
    return 'The object value was expected to validate against the defined property subschemas';
  }

  if (opcode === LOOP_PROPERTIES_TYPE ||
      opcode === LOOP_PROPERTIES_TYPE_EVALUATE ||
      opcode === LOOP_PROPERTIES_TYPE_STRICT ||
      opcode === LOOP_PROPERTIES_TYPE_STRICT_EVALUATE) {
    return 'The object properties were expected to be of type ' + typeName(value);
  }

  if (opcode === LOOP_PROPERTIES_TYPE_STRICT_ANY) {
    return 'The object properties were expected to be of type ' +
      describeTypeList(value);
  }

  if (opcode === LOOP_PROPERTIES_TYPE_STRICT_ANY_EVALUATE) {
    return 'The object properties were expected to be of type ' +
      describeTypeList(value);
  }

  if (opcode === LOOP_KEYS) {
    const size = objectSize(target);
    const keys = objectKeys(target);
    if (size === 0) {
      return 'The object is empty and no properties were expected to ' +
        'validate against the given subschema';
    }
    if (size === 1) {
      return 'The object property ' + escapeString(keys[0]) +
        ' was expected to validate against the given subschema';
    }
    let message = 'The object properties ';
    for (let index = 0; index < keys.length; index++) {
      if (index === keys.length - 1) {
        message += 'and ' + escapeString(keys[index]);
      } else {
        message += escapeString(keys[index]) + ', ';
      }
    }
    message += ' were expected to validate against the given subschema';
    return message;
  }

  if (opcode === LOOP_ITEMS) {
    return 'Every item in the array value was expected to validate against the given subschema';
  }

  if (opcode === LOOP_ITEMS_FROM) {
    let message = 'Every item in the array value';
    if (value === 1) {
      message += ' except for the first one';
    } else if (value > 0) {
      message += ' except for the first ' + value;
    }
    message += ' was expected to validate against the given subschema';
    return message;
  }

  if (opcode === LOOP_ITEMS_UNEVALUATED) {
    return 'The array items not covered by other array keywords, if any, ' +
      'were expected to validate against this subschema';
  }

  if (opcode === LOOP_ITEMS_TYPE || opcode === LOOP_ITEMS_TYPE_STRICT) {
    return 'The array items were expected to be of type ' + typeName(value);
  }

  if (opcode === LOOP_ITEMS_TYPE_STRICT_ANY) {
    return 'The array items were expected to be of type ' + describeTypeList(value);
  }

  if (opcode === LOOP_CONTAINS) {
    const minimum = value[0];
    const maximum = value[1];
    let plural = true;
    let message = 'The array value was expected to contain ';
    if (maximum !== null) {
      if (minimum === maximum && minimum === 0) {
        message += 'any number of';
      } else if (minimum === maximum) {
        message += 'exactly ' + minimum;
        if (minimum === 1) plural = false;
      } else if (minimum === 0) {
        message += 'up to ' + maximum;
        if (maximum === 1) plural = false;
      } else {
        message += minimum + ' to ' + maximum;
        if (maximum === 1) plural = false;
      }
    } else {
      message += 'at least ' + minimum;
      if (minimum === 1) plural = false;
    }
    message += plural
      ? ' items that validate against the given subschema'
      : ' item that validates against the given subschema';
    return message;
  }

  if (opcode === ASSERTION_DEFINES) {
    return 'The object value was expected to define the property ' +
      escapeString(value);
  }

  if (opcode === ASSERTION_DEFINES_STRICT) {
    return 'The value was expected to be an object that defines the property ' +
      escapeString(value);
  }

  if (opcode === ASSERTION_DEFINES_ALL) {
    let message = 'The object value was expected to define properties ';
    for (let index = 0; index < value.length; index++) {
      if (index === value.length - 1) {
        message += 'and ' + escapeString(value[index]);
      } else {
        message += escapeString(value[index]) + ', ';
      }
    }
    if (valid) return message;

    const missing = [];
    for (let index = 0; index < value.length; index++) {
      if (!Object.hasOwn(target, value[index])) {
        missing.push(value[index]);
      }
    }
    missing.sort();

    if (missing.length === 1) {
      message += ' but did not define the property ' + escapeString(missing[0]);
    } else {
      message += ' but did not define properties ';
      for (let index = 0; index < missing.length; index++) {
        if (index === missing.length - 1) {
          message += 'and ' + escapeString(missing[index]);
        } else {
          message += escapeString(missing[index]) + ', ';
        }
      }
    }
    return message;
  }

  if (opcode === ASSERTION_DEFINES_ALL_STRICT) {
    let message = 'The value was expected to be an object that defines properties ';
    for (let index = 0; index < value.length; index++) {
      if (index === value.length - 1) {
        message += 'and ' + escapeString(value[index]);
      } else {
        message += escapeString(value[index]) + ', ';
      }
    }
    return message;
  }

  if (opcode === ASSERTION_DEFINES_EXACTLY) {
    const sorted = [...value].sort();
    let message = 'The object value was expected to only define properties ';
    for (let index = 0; index < sorted.length; index++) {
      if (index === sorted.length - 1) {
        message += 'and ' + escapeString(sorted[index]);
      } else {
        message += escapeString(sorted[index]) + ', ';
      }
    }
    return message;
  }

  if (opcode === ASSERTION_DEFINES_EXACTLY_STRICT) {
    const sorted = [...value].sort();
    let message =
      'The value was expected to be an object that only defines properties ';
    for (let index = 0; index < sorted.length; index++) {
      if (index === sorted.length - 1) {
        message += 'and ' + escapeString(sorted[index]);
      } else {
        message += escapeString(sorted[index]) + ', ';
      }
    }
    return message;
  }

  if (opcode === ASSERTION_DEFINES_EXACTLY_STRICT_HASH3) {
    const entries = value[0];
    return 'The value was expected to be an object that only defines the ' +
      entries.length + ' given properties';
  }

  if (opcode === ASSERTION_TYPE || opcode === ASSERTION_TYPE_STRICT) {
    if (isWithinKeyword(evaluatePath, 'propertyNames') &&
        instanceLocation !== '') {
      const propertyName = lastInstanceToken(instanceLocation);
      return 'The property name ' + escapeString(propertyName) +
        ' was expected to be of type ' + typeName(value);
    }
    return describeTypeCheck(valid, targetType, value);
  }

  if (opcode === ASSERTION_TYPE_ANY) {
    return describeTypesCheck(valid, targetType, value);
  }

  if (opcode === ASSERTION_TYPE_STRICT_ANY) {
    return describeTypesCheck(valid, targetType, value);
  }

  if (opcode === ASSERTION_TYPE_STRING_BOUNDED) {
    const minimum = value[0];
    const maximum = value[1];
    if (minimum === 0 && maximum !== null) {
      return 'The value was expected to consist of a string of at most ' +
        maximum + (maximum === 1 ? ' character' : ' characters');
    }
    if (maximum !== null) {
      return 'The value was expected to consist of a string of ' + minimum +
        ' to ' + maximum + (maximum === 1 ? ' character' : ' characters');
    }
    return 'The value was expected to consist of a string of at least ' +
      minimum + (minimum === 1 ? ' character' : ' characters');
  }

  if (opcode === ASSERTION_TYPE_STRING_UPPER) {
    return 'The value was expected to consist of a string of at most ' +
      value + (value === 1 ? ' character' : ' characters');
  }

  if (opcode === ASSERTION_TYPE_ARRAY_BOUNDED) {
    const minimum = value[0];
    const maximum = value[1];
    if (minimum === 0 && maximum !== null) {
      return 'The value was expected to consist of an array of at most ' +
        maximum + (maximum === 1 ? ' item' : ' items');
    }
    if (maximum !== null) {
      return 'The value was expected to consist of an array of ' + minimum +
        ' to ' + maximum + (maximum === 1 ? ' item' : ' items');
    }
    return 'The value was expected to consist of an array of at least ' +
      minimum + (minimum === 1 ? ' item' : ' items');
  }

  if (opcode === ASSERTION_TYPE_ARRAY_UPPER) {
    return 'The value was expected to consist of an array of at most ' +
      value + (value === 1 ? ' item' : ' items');
  }

  if (opcode === ASSERTION_TYPE_OBJECT_BOUNDED) {
    const minimum = value[0];
    const maximum = value[1];
    if (minimum === 0 && maximum !== null) {
      return 'The value was expected to consist of an object of at most ' +
        maximum + (maximum === 1 ? ' property' : ' properties');
    }
    if (maximum !== null) {
      return 'The value was expected to consist of an object of ' + minimum +
        ' to ' + maximum + (maximum === 1 ? ' property' : ' properties');
    }
    return 'The value was expected to consist of an object of at least ' +
      minimum + (minimum === 1 ? ' property' : ' properties');
  }

  if (opcode === ASSERTION_TYPE_OBJECT_UPPER) {
    return 'The value was expected to consist of an object of at most ' +
      value + (value === 1 ? ' property' : ' properties');
  }

  if (opcode === ASSERTION_REGEX) {
    const pattern = value.source;
    if (isWithinKeyword(evaluatePath, 'propertyNames') &&
        instanceLocation !== '') {
      const propertyName = lastInstanceToken(instanceLocation);
      return 'The property name ' + escapeString(propertyName) +
        ' was expected to match the regular expression ' +
        escapeString(pattern);
    }
    return 'The string value ' + escapeString(target) +
      ' was expected to match the regular expression ' +
      escapeString(pattern);
  }

  if (opcode === ASSERTION_STRING_SIZE_LESS) {
    if (keyword === 'maxLength') {
      const maximum = value - 1;
      let message = '';
      if (isWithinKeyword(evaluatePath, 'propertyNames')) {
        const propertyName = lastInstanceToken(instanceLocation);
        message += 'The object property name ' + escapeString(propertyName);
        message += ' was expected to consist of at most ' + maximum +
          (maximum === 1 ? ' character' : ' characters');
        message += valid ? ' and' : ' but';
        message += ' it consisted of ';
        const length = unicodeLength(propertyName);
        message += length + (length === 1 ? ' character' : ' characters');
      } else {
        message += 'The string value ' + stringifyValue(target);
        message += ' was expected to consist of at most ' + maximum +
          (maximum === 1 ? ' character' : ' characters');
        message += valid ? ' and' : ' but';
        message += ' it consisted of ';
        const length = unicodeLength(target);
        message += length + (length === 1 ? ' character' : ' characters');
      }
      return message;
    }
    return '<unknown>';
  }

  if (opcode === ASSERTION_STRING_SIZE_GREATER) {
    if (keyword === 'minLength') {
      const minimum = value + 1;
      let message = '';
      if (isWithinKeyword(evaluatePath, 'propertyNames')) {
        const propertyName = lastInstanceToken(instanceLocation);
        message += 'The object property name ' + escapeString(propertyName);
        message += ' was expected to consist of at least ' + minimum +
          (minimum === 1 ? ' character' : ' characters');
        message += valid ? ' and' : ' but';
        message += ' it consisted of ';
        const length = unicodeLength(propertyName);
        message += length + (length === 1 ? ' character' : ' characters');
      } else {
        message += 'The string value ' + stringifyValue(target);
        message += ' was expected to consist of at least ' + minimum +
          (minimum === 1 ? ' character' : ' characters');
        message += valid ? ' and' : ' but';
        message += ' it consisted of ';
        const length = unicodeLength(target);
        message += length + (length === 1 ? ' character' : ' characters');
      }
      return message;
    }
    return '<unknown>';
  }

  if (opcode === ASSERTION_ARRAY_SIZE_LESS) {
    if (keyword === 'maxItems') {
      const maximum = value - 1;
      let message = 'The array value was expected to contain at most ' +
        maximum + (maximum === 1 ? ' item' : ' items');
      message += valid ? ' and' : ' but';
      message += ' it contained ' + target.length +
        (target.length === 1 ? ' item' : ' items');
      return message;
    }
    return '<unknown>';
  }

  if (opcode === ASSERTION_ARRAY_SIZE_GREATER) {
    const minimum = value + 1;
    let message = 'The array value was expected to contain at least ' +
      minimum + (minimum === 1 ? ' item' : ' items');
    message += valid ? ' and' : ' but';
    message += ' it contained ' + target.length +
      (target.length === 1 ? ' item' : ' items');
    return message;
  }

  if (opcode === ASSERTION_OBJECT_SIZE_LESS) {
    if (keyword === 'additionalProperties') {
      return 'The object value was not expected to define additional properties';
    }
    if (keyword === 'maxProperties') {
      const maximum = value - 1;
      const size = objectSize(target);
      let message = 'The object value was expected to contain at most ' +
        maximum + (maximum === 1 ? ' property' : ' properties');
      message += valid ? ' and' : ' but';
      message += ' it contained ' + size;
      if (size === 0) {
        message += ' properties';
      } else if (size === 1) {
        message += ' property: ' + escapeString(objectKeys(target)[0]);
      } else {
        message += ' properties: ';
        const properties = objectKeys(target).sort();
        message += formatList(properties.map(escapeString), 'and');
      }
      return message;
    }
    return '<unknown>';
  }

  if (opcode === ASSERTION_OBJECT_SIZE_GREATER) {
    if (keyword === 'minProperties') {
      const minimum = value + 1;
      const size = objectSize(target);
      let message = 'The object value was expected to contain at least ' +
        minimum + (minimum === 1 ? ' property' : ' properties');
      message += valid ? ' and' : ' but';
      message += ' it contained ' + size;
      if (size === 1) {
        message += ' property: ' + escapeString(objectKeys(target)[0]);
      } else {
        message += ' properties: ';
        const properties = objectKeys(target).sort();
        message += formatList(properties.map(escapeString), 'and');
      }
      return message;
    }
    return '<unknown>';
  }

  if (opcode === ASSERTION_EQUAL) {
    let message = '';
    if (isWithinKeyword(evaluatePath, 'propertyNames') &&
        instanceLocation !== '') {
      const propertyName = lastInstanceToken(instanceLocation);
      message += 'The property name ' + escapeString(propertyName);
    } else {
      message += 'The ' + typeName(targetType) + ' value ';
      message += stringifyValue(target);
    }
    message += ' was expected to equal the ' + typeName(jsonTypeOf(value)) +
      ' constant ' + stringifyValue(value);
    return message;
  }

  if (opcode === ASSERTION_EQUALS_ANY) {
    let message = '';
    if (isWithinKeyword(evaluatePath, 'propertyNames') &&
        instanceLocation !== '') {
      const propertyName = lastInstanceToken(instanceLocation);
      message += 'The property name ' + escapeString(propertyName);
    } else {
      message += 'The ' + typeName(targetType) + ' value ';
      message += stringifyValue(target);
    }

    const values = Array.isArray(value) ? value : (value.values || []);
    if (values.length === 1) {
      message += ' was expected to equal the ' +
        typeName(jsonTypeOf(values[0])) + ' constant ' +
        stringifyValue(values[0]);
    } else {
      if (valid) {
        message += ' was expected to equal one of the ' +
          values.length + ' declared values';
      } else {
        message += ' was expected to equal one of the following values: ';
        const sorted = [...values].sort(jsonCompare);
        for (let index = 0; index < sorted.length; index++) {
          if (index === sorted.length - 1) {
            message += 'and ' + stringifyValue(sorted[index]);
          } else {
            message += stringifyValue(sorted[index]) + ', ';
          }
        }
      }
    }
    return message;
  }

  if (opcode === ASSERTION_EQUALS_ANY_STRING_HASH) {
    let message = '';
    if (isWithinKeyword(evaluatePath, 'propertyNames') &&
        instanceLocation !== '') {
      const propertyName = lastInstanceToken(instanceLocation);
      message += 'The property name ' + escapeString(propertyName);
    } else {
      message += 'The ' + typeName(targetType) + ' value ';
      message += stringifyValue(target);
    }
    const entries = value[0];
    if (entries.length === 1) {
      message += ' was expected to equal the given constant';
    } else {
      message += ' was expected to equal one of the given declared values';
    }
    return message;
  }

  if (opcode === ASSERTION_GREATER_EQUAL) {
    return 'The ' + valueTypeName(target) + ' value ' +
      stringifyValue(target) +
      ' was expected to be greater than or equal to the ' +
      valueTypeName(value) + ' ' + stringifyValue(value);
  }

  if (opcode === ASSERTION_LESS_EQUAL) {
    return 'The ' + valueTypeName(target) + ' value ' +
      stringifyValue(target) +
      ' was expected to be less than or equal to the ' +
      valueTypeName(value) + ' ' + stringifyValue(value);
  }

  if (opcode === ASSERTION_GREATER) {
    let message = 'The ' + valueTypeName(target) + ' value ' +
      stringifyValue(target) +
      ' was expected to be greater than the ' +
      valueTypeName(value) + ' ' + stringifyValue(value);
    if (!valid && jsonEqual(value, target)) {
      message += ', but they were equal';
    }
    return message;
  }

  if (opcode === ASSERTION_LESS) {
    let message = 'The ' + valueTypeName(target) + ' value ' +
      stringifyValue(target) +
      ' was expected to be less than the ' +
      valueTypeName(value) + ' ' + stringifyValue(value);
    if (!valid && jsonEqual(value, target)) {
      message += ', but they were equal';
    }
    return message;
  }

  if (opcode === ASSERTION_UNIQUE) {
    if (valid) {
      return 'The array value was expected to not contain duplicate items';
    }
    const duplicateStrs = new Set();
    for (let index = 0; index < target.length; index++) {
      for (let other = index + 1; other < target.length; other++) {
        if (jsonEqual(target[index], target[other])) {
          duplicateStrs.add(stringifyValue(target[index]));
        }
      }
    }
    const sorted = [...duplicateStrs].sort();
    if (sorted.length === 1) {
      return 'The array value contained the following duplicate item: ' + sorted[0];
    }
    let message = 'The array value contained the following duplicate items: ';
    for (let index = 0; index < sorted.length; index++) {
      if (index === sorted.length - 1) {
        message += 'and ' + sorted[index];
      } else {
        message += sorted[index] + ', ';
      }
    }
    return message;
  }

  if (opcode === ASSERTION_DIVISIBLE) {
    return 'The ' + valueTypeName(target) + ' value ' +
      stringifyValue(target) +
      ' was expected to be divisible by the ' +
      valueTypeName(value) + ' ' + stringifyValue(value);
  }

  if (opcode === ASSERTION_STRING_TYPE) {
    return 'The string value ' + escapeString(target) +
      ' was expected to represent a valid URI';
  }

  if (opcode === ASSERTION_PROPERTY_TYPE ||
      opcode === ASSERTION_PROPERTY_TYPE_EVALUATE ||
      opcode === ASSERTION_PROPERTY_TYPE_STRICT ||
      opcode === ASSERTION_PROPERTY_TYPE_STRICT_EVALUATE) {
    return describeTypeCheck(valid, targetType, value);
  }

  if (opcode === ASSERTION_PROPERTY_TYPE_STRICT_ANY ||
      opcode === ASSERTION_PROPERTY_TYPE_STRICT_ANY_EVALUATE) {
    return describeTypesCheck(valid, targetType, value);
  }

  if (opcode === ASSERTION_ARRAY_PREFIX ||
      opcode === ASSERTION_ARRAY_PREFIX_EVALUATE) {
    const childCount = children ? children.length : 0;
    let message = 'The first ';
    if (childCount <= 2) {
      message += 'item of the array value was';
    } else {
      message += (childCount - 1) + ' items of the array value were';
    }
    message += ' expected to validate against the corresponding subschemas';
    return message;
  }

  if (opcode === LOOP_PROPERTIES_MATCH) {
    const childCount = children ? children.length : 0;
    let message = 'The object value was expected to validate against the ';
    if (childCount === 1) {
      message += 'single defined property subschema';
    } else {
      message += childCount + ' defined properties subschemas';
    }
    return message;
  }

  if (opcode === LOOP_PROPERTIES_MATCH_CLOSED) {
    const childCount = children ? children.length : 0;
    if (childCount === 1) {
      return 'The object value was expected to validate against the ' +
        'single defined property subschema';
    }
    return 'Every object value was expected to validate against one of the ' +
      childCount + ' defined properties subschemas';
  }

  if (opcode === LOGICAL_WHEN_DEFINES) {
    return 'The object value defined the property "' + value + '"';
  }

  if (opcode === LOOP_PROPERTIES_REGEX) {
    return 'The object properties that match the regular expression "' +
      value.source + '" were expected to validate against the defined ' +
      'pattern property subschema';
  }

  if (opcode === LOOP_PROPERTIES_REGEX_CLOSED) {
    return 'The object properties were expected to match the regular ' +
      'expression "' + value.source + '" and validate against the ' +
      'defined pattern property subschema';
  }

  if (opcode === LOOP_PROPERTIES_STARTS_WITH) {
    return 'The object properties that start with the string "' + value +
      '" were expected to validate against the defined pattern property subschema';
  }

  if (opcode === LOGICAL_WHEN_TYPE) {
    if (keyword === 'items') {
      return describeTypeCheck(valid, targetType, value);
    }

    if (keyword === 'properties') {
      const childCount = children ? children.length : 0;
      if (targetType !== TYPE_OBJECT) {
        return describeTypeCheck(valid, targetType, TYPE_OBJECT);
      }
      let message = 'The object value was expected to validate against the ';
      if (childCount === 1) {
        message += 'single defined property subschema';
      } else {
        message += 'defined properties subschemas';
      }
      return message;
    }

    if (keyword === 'dependencies') {
      const childCount = children ? children.length : 0;
      const present = new Set();
      const presentWithSchemas = new Set();
      const presentWithProperties = new Set();
      const allDependencies = new Set();
      const requiredProperties = new Set();

      for (let index = 0; index < childCount; index++) {
        const child = children[index];
        if (child[0] === LOGICAL_WHEN_DEFINES) {
          const property = child[5];
          allDependencies.add(property);
          if (!Object.hasOwn(target, property)) continue;
          present.add(property);
          presentWithSchemas.add(property);
        } else if (child[0] === ASSERTION_PROPERTY_DEPENDENCIES) {
          const entries = child[5];
          for (const property in entries) {
            allDependencies.add(property);
            if (Object.hasOwn(target, property)) {
              present.add(property);
              presentWithProperties.add(property);
              const dependencies = entries[property];
              for (let depIndex = 0; depIndex < dependencies.length; depIndex++) {
                if (valid || !Object.hasOwn(target, dependencies[depIndex])) {
                  requiredProperties.add(dependencies[depIndex]);
                }
              }
            }
          }
        }
      }

      const allDepsArr = [...allDependencies].sort();
      const presentArr = [...present].sort();
      const presentSchemasArr = [...presentWithSchemas].sort();
      const requiredArr = [...requiredProperties].sort();

      if (presentWithSchemas.size === 0 && presentWithProperties.size === 0) {
        let message = 'The object value did not define the';
        if (allDepsArr.length === 1) {
          message += ' property ' + escapeString(allDepsArr[0]);
        } else {
          message += ' properties ';
          message += formatList(allDepsArr.map(escapeString), 'or');
        }
        return message;
      }

      let message = '';
      if (presentArr.length === 1) {
        message += 'Because the object value defined the';
        message += ' property ' + escapeString(presentArr[0]);
      } else {
        message += 'Because the object value defined the';
        message += ' properties ';
        message += formatList(presentArr.map(escapeString), 'and');
      }

      if (requiredArr.length > 0) {
        message += ', it was also expected to define the';
        if (requiredArr.length === 1) {
          message += ' property ' + escapeString(requiredArr[0]);
        } else {
          message += ' properties ';
          message += formatList(requiredArr.map(escapeString), 'and');
        }
      }

      if (presentSchemasArr.length > 0) {
        message += ', ';
        if (requiredArr.length > 0) message += 'and ';
        message += 'it was also expected to successfully validate against ' +
          'the corresponding ';
        if (presentSchemasArr.length === 1) {
          message += escapeString(presentSchemasArr[0]) + ' subschema';
        } else {
          message += formatList(presentSchemasArr.map(escapeString), 'and');
          message += ' subschemas';
        }
      }

      return message;
    }

    if (keyword === 'dependentSchemas') {
      const childCount = children ? children.length : 0;
      const present = new Set();
      const allDependencies = new Set();

      for (let index = 0; index < childCount; index++) {
        const child = children[index];
        const property = child[5];
        allDependencies.add(property);
        if (Object.hasOwn(target, property)) {
          present.add(property);
        }
      }

      const allDepsArr = [...allDependencies].sort();
      const presentArr = [...present].sort();

      if (present.size === 0) {
        let message = 'The object value did not define the';
        if (allDepsArr.length === 1) {
          message += ' property ' + escapeString(allDepsArr[0]);
        } else {
          message += ' properties ';
          message += formatList(allDepsArr.map(escapeString), 'or');
        }
        return message;
      }

      if (present.size === 1) {
        return 'Because the object value defined the property ' +
          escapeString(presentArr[0]) +
          ', it was also expected to validate against the corresponding subschema';
      }

      let message = 'Because the object value defined the properties ';
      message += formatList(presentArr.map(escapeString), 'and');
      message += ', it was also expected to validate against the ' +
        'corresponding subschemas';
      return message;
    }

    return '<unknown>';
  }

  if (opcode === ASSERTION_PROPERTY_DEPENDENCIES) {
    const present = [];
    const allDependencies = [];
    const required = new Set();

    for (const property in value) {
      allDependencies.push(property);
      if (Object.hasOwn(target, property)) {
        present.push(property);
        const dependencies = value[property];
        for (let index = 0; index < dependencies.length; index++) {
          if (valid || !Object.hasOwn(target, dependencies[index])) {
            required.add(dependencies[index]);
          }
        }
      }
    }

    allDependencies.sort();
    present.sort();
    const requiredArr = [...required].sort();

    if (present.length === 0) {
      let message = 'The object value did not define the';
      if (allDependencies.length === 1) {
        message += ' property ' + escapeString(allDependencies[0]);
      } else {
        message += ' properties ';
        message += formatList(allDependencies.map(escapeString), 'or');
      }
      return message;
    }

    let message = '';
    if (present.length === 1) {
      message += 'Because the object value defined the';
      message += ' property ' + escapeString(present[0]);
    } else {
      message += 'Because the object value defined the';
      message += ' properties ';
      message += formatList(present.map(escapeString), 'and');
    }

    message += ', it was also expected to define the';
    if (requiredArr.length === 1) {
      message += ' property ' + escapeString(requiredArr[0]);
    } else {
      message += ' properties ';
      message += formatList(requiredArr.map(escapeString), 'and');
    }
    return message;
  }

  if (opcode === LOGICAL_WHEN_ARRAY_SIZE_GREATER) {
    if (keyword === 'additionalItems' || keyword === 'items') {
      if (target.length > value) {
        const rest = target.length - value;
        return 'The array value contains ' + rest + ' additional' +
          (rest === 1 ? ' item' : ' items') +
          ' not described by related keywords';
      }
      return 'The array value does not contain additional items not ' +
        'described by related keywords';
    }
    return '<unknown>';
  }

  if (opcode === CONTROL_JUMP) {
    if (isWithinKeyword(evaluatePath, 'propertyNames') &&
        instanceLocation !== '') {
      return 'The string value was expected to validate against the referenced schema';
    }
    return describeReference(target);
  }

  return '<unknown>';
}
