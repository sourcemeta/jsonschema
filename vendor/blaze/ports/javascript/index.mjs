const JSON_VERSION = 4;
const DEPTH_LIMIT = 300;
const ANNOTATION_EMIT = 49;
const ANNOTATION_TO_PARENT = 50;
const ANNOTATION_BASENAME_TO_PARENT = 51;
const CONTROL_GROUP_START = 92;
const CONTROL_EVALUATE_END = 96;
const URI_REGEX = /^[a-zA-Z][a-zA-Z0-9+\-.]*:[^\s]*$/;

function buildJsonPointer(tokens, length) {
  if (length === 0) return '';
  let result = '';
  for (let index = 0; index < length; index++) {
    const token = String(tokens[index]);
    result += '/' + token.replaceAll('~', '~0').replaceAll('/', '~1');
  }
  return result;
}

const Type = {
  Null: 0,
  Boolean: 1,
  Integer: 2,
  Real: 3,
  String: 4,
  Array: 5,
  Object: 6,
  Decimal: 7
};

function jsonTypeOf(value) {
  if (value === null) return Type.Null;
  switch (typeof value) {
    case 'boolean': return Type.Boolean;
    case 'number': return Number.isInteger(value) ? Type.Integer : Type.Real;
    case 'bigint': return Type.Integer;
    case 'string': return Type.String;
    case 'object': return Array.isArray(value) ? Type.Array : Type.Object;
    default: return Type.Null;
  }
}

function isIntegral(value) {
  return typeof value === 'bigint' ||
    (typeof value === 'number' && Number.isFinite(value) && Math.floor(value) === value);
}

function resolveInstance(instance, relativeInstanceLocation) {
  const length = relativeInstanceLocation.length;
  if (length === 0) return instance;
  if (length === 1) {
    if (instance === undefined || instance === null) return undefined;
    return instance[relativeInstanceLocation[0]];
  }
  let current = instance;
  for (let index = 0; index < length; index++) {
    if (current === undefined || current === null) return undefined;
    current = current[relativeInstanceLocation[index]];
  }
  return current;
}

function prepareInstruction(instruction) {
  const wrapper = instruction[5];
  if (wrapper === undefined || wrapper === null) {
    instruction[5] = null;
  } else {
    const typeIndex = wrapper[0];
    if (typeIndex === 0 || wrapper.length === 1) {
      instruction[5] = null;
    } else {
      const payload = wrapper[1];
      switch (typeIndex) {
        case 4:
          instruction[5] = payload[0];
          break;
        case 9:
          try { instruction[5] = new RegExp(payload, 'u'); }
          catch { instruction[5] = new RegExp(payload); }
          break;
        case 13: {
          if (Array.isArray(payload)) {
            const object = Object.create(null);
            for (let index = 0; index < payload.length; index++) {
              object[payload[index][0]] = payload[index][1];
            }
            instruction[5] = object;
          } else {
            instruction[5] = payload;
          }
          break;
        }
        case 15: {
          if (Array.isArray(payload)) {
            const object = Object.create(null);
            for (let index = 0; index < payload.length; index++) {
              object[payload[index][0]] = payload[index][1];
            }
            instruction[5] = object;
          } else {
            instruction[5] = payload;
          }
          break;
        }
        case 16: {
          payload[0] = new Set(payload[0]);
          const regexes = payload[2];
          for (let index = 0; index < regexes.length; index++) {
            try { regexes[index] = new RegExp(regexes[index], 'u'); }
            catch { regexes[index] = new RegExp(regexes[index]); }
          }
          instruction[5] = payload;
          break;
        }
        default:
          instruction[5] = payload;
          break;
      }
    }
  }

  const opcode = instruction[0];
  if (opcode === 27 && Array.isArray(instruction[5])) {
    const values = instruction[5];
    let allPrimitive = true;
    for (let index = 0; index < values.length; index++) {
      const element = values[index];
      if (element !== null && typeof element === 'object') {
        allPrimitive = false;
        break;
      }
    }
    if (allPrimitive) {
      instruction[5] = { set: new Set(values), values, primitive: true };
    }
  }

  if (instruction.length < 7) {
    instruction.push(undefined);
  }

  instruction.push(handlers[opcode] || null);

  const children = instruction[6];
  if (children) {
    for (let index = 0; index < children.length; index++) {
      prepareInstruction(children[index]);
    }
  }
}

function resolveJumpTargets(instructions, targets) {
  for (let index = 0; index < instructions.length; index++) {
    const instruction = instructions[index];
    if (instruction[0] === 98) {
      const targetIndex = instruction[5];
      if (targetIndex < targets.length) {
        instruction[5] = targets[targetIndex];
      }
    }
    const children = instruction[6];
    if (children) {
      resolveJumpTargets(children, targets);
    }
  }
}

const FNV_OFFSET = 14695981039346656037n;
const FNV_PRIME = 1099511628211n;
const MASK_53 = (1n << 53n) - 1n;

function blazeHash(resource, fragment) {
  let result = FNV_OFFSET & MASK_53;
  for (let index = 0; index < fragment.length; index++) {
    result ^= BigInt(fragment.charCodeAt(index));
    result = (result * FNV_PRIME) & MASK_53;
  }
  return Number((BigInt(resource) + result) & MASK_53);
}

function collectAnchorNames(targets, result) {
  for (let targetIndex = 0; targetIndex < targets.length; targetIndex++) {
    collectAnchorNamesFromInstructions(targets[targetIndex], result);
  }
}

function collectAnchorNamesFromInstructions(instructions, result) {
  for (let index = 0; index < instructions.length; index++) {
    const instruction = instructions[index];
    if (instruction[0] === 97 && typeof instruction[5] === 'string') {
      result.add(instruction[5]);
    }
    if (instruction[6]) {
      collectAnchorNamesFromInstructions(instruction[6], result);
    }
  }
}

function compile(template) {
  const targets = template[3];
  for (let targetIndex = 0; targetIndex < targets.length; targetIndex++) {
    const target = targets[targetIndex];
    for (let index = 0; index < target.length; index++) {
      prepareInstruction(target[index]);
    }
  }

  for (let targetIndex = 0; targetIndex < targets.length; targetIndex++) {
    resolveJumpTargets(targets[targetIndex], targets);
  }

  const labels = new Map();
  const rawLabels = template[4];
  for (let index = 0; index < rawLabels.length; index++) {
    const pair = rawLabels[index];
    labels.set(pair[0], pair[1]);
  }

  const anchors = new Map();
  if (template[1]) {
    const anchorNames = new Set();
    collectAnchorNames(targets, anchorNames);
    const resourceCount = targets.length;
    for (const anchor of anchorNames) {
      for (let resource = 0; resource <= resourceCount; resource++) {
        const hash = blazeHash(resource, anchor);
        const targetIndex = labels.get(hash);
        if (targetIndex !== undefined) {
          anchors.set(resource + ':' + anchor, targets[targetIndex]);
        }
      }
    }
  }

  template[4] = labels;
  template[5] = anchors;
  return template;
}

function emitResolve(varName, instanceExpr, relInstance) {
  if (relInstance.length === 0) return `var ${varName}=${instanceExpr};`;
  if (relInstance.length === 1) return `var ${varName}=${instanceExpr}==null?void 0:${instanceExpr}[${JSON.stringify(relInstance[0])}];`;
  return null;
}

function compileInstructionToCode(instruction, captures, visited, budget) {
  if (budget[0] <= 0) { var ci = captures.length; captures.push(instruction); return 'return _fh['+instruction[0]+'](_c['+ci+'],i,d,_t,_v);'; }
  var opcode = instruction[0], ri = instruction[2], value = instruction[5], children = instruction[6];
  function R(v) { return emitResolve(v, 'i', ri); }
  function inlineCondition(child, tv) {
    var op = child[0], cr = child[2], cv = child[5];
    if (cr.length !== 0) return null;
    switch (op) {
      case 11: return '{if(_es(' + tv + ')!==' + cv + ')return false;}';
      case 42: return 'if(' + tv + '!=null&&typeof ' + tv + "==='object'&&!Array.isArray(" + tv + ')){' + emitResolve('t', tv, cr) + 'if(t!==void 0&&_es(t)!==' + cv + ')return false;}';
      default: return null;
    }
  }
  function seq(ci, tv) { var c = ''; for (var j = 0; j < ci.length; j++) { var inl = inlineCondition(ci[j], tv); if (inl !== null) { budget[0] -= inl.length; c += inl; continue; } var r = compileInstructionToCode(ci[j], captures, visited, budget); if (r === null) { var idx = captures.length; captures.push(ci[j]); c += 'if(!_e(_c['+idx+'],'+tv+',d+1,_t,_v))return false;'; } else { budget[0] -= r.length; c += 'if(!(function(i,d,_t,_v){'+r+'})('+tv+',d+1,_t,_v))return false;'; } } return c; }
  function lb(ci, iv) { return seq(ci, iv); }
  function fb(op) { var ci = captures.length; captures.push(instruction); return 'return _fh['+op+'](_c['+ci+'],i,d,_t,_v);'; }
  function cc(child, tv) { var r = compileInstructionToCode(child, captures, visited, budget); if (r === null) { var ci = captures.length; captures.push(child); return '_e(_c['+ci+'],'+tv+',d+1,_t,_v)'; } budget[0] -= r.length; return '(function(i,d,_t,_v){'+r+'})('+tv+',d+1,_t,_v)'; }
  var IO = "if(i==null||typeof i!=='object'||Array.isArray(i))return true;";
  var TO = "if(t==null||typeof t!=='object'||Array.isArray(t))";
  switch (opcode) {
    case 0: return 'return false;';
    case 1: { var r=R('t'); return r?r+TO+'return true;return Object.hasOwn(t,'+JSON.stringify(value)+');':null; }
    case 2: { var r=R('t'); return r?r+"return t!=null&&typeof t==='object'&&!Array.isArray(t)&&Object.hasOwn(t,"+JSON.stringify(value)+');':null; }
    case 3: { var r=R('t'); if(!r)return null; var c=r+TO+'return true;'; for(var j=0;j<value.length;j++)c+='if(!Object.hasOwn(t,'+JSON.stringify(value[j])+'))return false;'; return c+'return true;'; }
    case 4: { var r=R('t'); if(!r)return null; var c=r+TO+'return false;'; for(var j=0;j<value.length;j++)c+='if(!Object.hasOwn(t,'+JSON.stringify(value[j])+'))return false;'; return c+'return true;'; }
    case 5: { var r=R('t'); if(!r)return null; var c=r+TO+'return true;var s=0;for(var k in t)s++;if(s!=='+value.length+')return false;'; for(var j=0;j<value.length;j++)c+='if(!Object.hasOwn(t,'+JSON.stringify(value[j])+'))return false;'; return c+'return true;'; }
    case 6: { var r=R('t'); if(!r)return null; var c=r+TO+'return false;var s=0;for(var k in t)s++;if(s!=='+value.length+')return false;'; for(var j=0;j<value.length;j++)c+='if(!Object.hasOwn(t,'+JSON.stringify(value[j])+'))return false;'; return c+'return true;'; }
    case 7: return fb(7); case 8: return fb(8);
    case 9: { var r=R('t'); return r?r+'var a=_jt(t);if(a==='+value+')return true;return '+value+'===2&&_ii(t);':null; }
    case 10: { var r=R('t'); return r?r+'var a=_jt(t);if(('+value+'&(1<<a))!==0)return true;return('+value+'&4)!==0&&_ii(t);':null; }
    case 11: { var r=R('t'); return r?r+'return _es(t)==='+value+';':null; }
    case 12: { var r=R('t'); return r?r+'return('+value+'&(1<<_es(t)))!==0;':null; }
    case 13: { var r=R('t'); return r?r+"if(typeof t!=='string')return false;var l=_ul(t);if(l<"+value[0]+')return false;'+(value[1]!==null?'if(l>'+value[1]+')return false;':'')+'return true;':null; }
    case 14: { var r=R('t'); return r?r+"return typeof t==='string'&&_ul(t)<="+value+';':null; }
    case 15: { var r=R('t'); return r?r+'if(!Array.isArray(t))return false;if(t.length<'+value[0]+')return false;'+(value[1]!==null?'if(t.length>'+value[1]+')return false;':'')+'return true;':null; }
    case 16: { var r=R('t'); return r?r+'return Array.isArray(t)&&t.length<='+value+';':null; }
    case 17: { var r=R('t'); return r?r+TO+'return false;var s=0;for(var k in t)s++;if(s<'+value[0]+')return false;'+(value[1]!==null?'if(s>'+value[1]+')return false;':'')+'return true;':null; }
    case 18: { var r=R('t'); return r?r+TO+'return false;var s=0;for(var k in t)s++;return s<='+value+';':null; }
    case 19: return fb(19); case 20: return fb(20); case 21: return fb(21);
    case 22: { var r=R('t'); return r?r+'if(!Array.isArray(t))return true;return t.length<'+value+';':null; }
    case 23: { var r=R('t'); return r?r+'if(!Array.isArray(t))return true;return t.length>'+value+';':null; }
    case 24: { var r=R('t'); return r?r+TO+'return true;var s=0;for(var k in t)s++;return s<'+value+';':null; }
    case 25: { var r=R('t'); return r?r+TO+'return true;var s=0;for(var k in t)s++;return s>'+value+';':null; }
    case 26: { if(typeof value==='string'||typeof value==='number'||typeof value==='boolean'||value===null){var r=R('t');return r?r+'return t==='+JSON.stringify(value)+';':null;}return fb(26); }
    case 27: return fb(27); case 28: return fb(28);
    case 29: { var r=R('t'),v=typeof value==='bigint'?value+'n':value; return r?r+"var tt=typeof t;if(tt!=='number'&&tt!=='bigint')return true;return t>="+v+';':null; }
    case 30: { var r=R('t'),v=typeof value==='bigint'?value+'n':value; return r?r+"var tt=typeof t;if(tt!=='number'&&tt!=='bigint')return true;return t<="+v+';':null; }
    case 31: { var r=R('t'),v=typeof value==='bigint'?value+'n':value; return r?r+"var tt=typeof t;if(tt!=='number'&&tt!=='bigint')return true;return t>"+v+';':null; }
    case 32: { var r=R('t'),v=typeof value==='bigint'?value+'n':value; return r?r+"var tt=typeof t;if(tt!=='number'&&tt!=='bigint')return true;return t<"+v+';':null; }
    case 33: return fb(33); case 34: return fb(34);
    case 35: return fb(35); case 36: return fb(36); case 37: return fb(37); case 38: return fb(38);
    case 39: return fb(39);
    case 40: { var r=R('t'); return r?IO+r+'if(t===void 0)return true;var a=_jt(t);return a==='+value+'||('+value+'===2&&_ii(t));':null; }
    case 41: return fb(41);
    case 42: { var r=R('t'); return r?IO+r+'if(t===void 0)return true;return _es(t)==='+value+';':null; }
    case 43: return fb(43);
    case 44: { var r=R('t'); return r?IO+r+'if(t===void 0)return true;return('+value+'&(1<<_es(t)))!==0;':null; }
    case 45: return fb(45); case 46: return fb(46); case 47: return fb(47);
    case 48: return fb(48);
    case 49: return 'return true;'; case 50: return 'return true;'; case 51: return 'return true;'; case 52: return 'return true;';
    case 53: { var r=R('t'); if(!r)return null; if(!children||children.length===0)return 'return false;'; var c=r; for(var j=0;j<children.length;j++)c+='if(!'+cc(children[j],'t')+')return true;'; return c+'return false;'; }
    case 54: return fb(54);
    case 55: { var r=R('t'); if(!r)return null; if(!children||children.length===0)return 'return true;'; if(value){var c=r+'var __r=false;';for(var j=0;j<children.length;j++)c+='if('+cc(children[j],'t')+')__r=true;';return c+'return __r;';} var c=r;for(var j=0;j<children.length;j++)c+='if('+cc(children[j],'t')+')return true;';return c+'return false;'; }
    case 56: { var r=R('t'); if(!r)return null; if(!children||children.length===0)return 'return true;'; return r+seq(children,'t')+'return true;'; }
    case 57: { var r=R('t'); if(!r)return null; if(!children||children.length===0)return 'return false;'; var c=r+'var __r=true,__m=false;';for(var j=0;j<children.length;j++){c+='if('+cc(children[j],'t')+'){if(__m){__r=false;'+(!value?'return false;':'')+ '}else __m=true;}';}return c+'return __r&&__m;'; }
    case 58: return fb(58);
    case 59: { var r=R('t'); if(!r)return null; var c=r+'if(_jt(t)!=='+value+')return true;'; if(children&&children.length>0)c+=seq(children,'t'); return c+'return true;'; }
    case 60: { var r=R('t'); if(!r)return null; var c=r+TO+'return true;if(!Object.hasOwn(t,'+JSON.stringify(value)+'))return true;'; if(children&&children.length>0)c+=seq(children,'t'); return c+'return true;'; }
    case 61: { var r=R('t'); if(!r)return null; var c=r+'if(!Array.isArray(t)||t.length<='+value+')return true;'; if(children&&children.length>0)c+=seq(children,'t'); return c+'return true;'; }
    case 62: return fb(62); case 63: return fb(63);
    case 64: { var r=R('t'); if(!r)return null; if(!children||children.length===0)return r+'return true;'; var mi=captures.length; captures.push(value); var gf=''; for(var gi=0;gi<children.length;gi++){var gc=children[gi][6]; if(gc&&gc.length>0){gf+='function(i,d,_t,_v){'+lb(gc,'i')+'return true;},';}else{gf+='null,';}} return r+TO+'return true;var __cg=['+gf+'];for(var k in t){var __mi=_c['+mi+'][k];if(__mi!==void 0){var __cf=__cg[__mi];if(__cf&&!__cf(t,d,_t,_v))return false;}}return true;'; }
    case 65: { var r=R('t'); if(!r)return null; if(!children||children.length===0)return r+'return true;'; var mi=captures.length; captures.push(value); var gf=''; for(var gi=0;gi<children.length;gi++){var gc=children[gi][6]; if(gc&&gc.length>0){gf+='function(i,d,_t,_v){'+lb(gc,'i')+'return true;},';}else{gf+='null,';}} return r+TO+'return true;var __cg=['+gf+'];for(var k in t){var __mi=_c['+mi+'][k];if(__mi===void 0)return false;var __cf=__cg[__mi];if(__cf&&!__cf(t,d,_t,_v))return false;}return true;'; }
    case 66: { var r=R('t'); if(!r)return null; if(!children||children.length===0)return r+'return true;'; return r+TO+'return true;for(var k in t){'+lb(children,'t[k]')+'}return true;'; }
    case 67: return fb(67); case 68: return fb(68); case 69: return fb(69); case 70: return fb(70); case 71: return fb(71);
    case 72: return fb(72); case 73: return fb(73); case 74: return fb(74); case 75: return fb(75);
    case 76: { var r=R('t'); return r?r+TO+'return true;for(var k in t){if(_es(t[k])!=='+value+')return false;}return true;':null; }
    case 77: return fb(77); case 78: return fb(78); case 79: return fb(79); case 80: return fb(80);
    case 81: { var r=R('t'); if(!r)return null; if(!children||children.length===0)return r+'return true;'; return r+'if(!Array.isArray(t))return true;for(var j=0;j<t.length;j++){'+lb(children,'t[j]')+'}return true;'; }
    case 82: { var r=R('t'); if(!r)return null; if(!children||children.length===0)return r+'return true;'; return r+'if(!Array.isArray(t)||'+value+'>=t.length)return true;for(var j='+value+';j<t.length;j++){'+lb(children,'t[j]')+'}return true;'; }
    case 83: return fb(83);
    case 84: { var r=R('t'); return r?r+'if(!Array.isArray(t))return true;for(var j=0;j<t.length;j++){var a=_jt(t[j]);if(a!=='+value+'&&!('+value+'===2&&_ii(t[j])))return false;}return true;':null; }
    case 85: { var r=R('t'); return r?r+'if(!Array.isArray(t))return true;for(var j=0;j<t.length;j++){if(_es(t[j])!=='+value+')return false;}return true;':null; }
    case 86: { var r=R('t'); return r?r+'if(!Array.isArray(t))return true;for(var j=0;j<t.length;j++){if(('+value+'&(1<<_es(t[j])))===0)return false;}return true;':null; }
    case 87: return fb(87); case 88: return fb(88); case 89: return fb(89); case 90: return fb(90); case 91: return fb(91);
    case 92: { if(!children||children.length===0)return 'return true;'; var c=''; for(var j=0;j<children.length;j++){var r2=compileInstructionToCode(children[j],captures,visited,budget); if(r2===null){var ci=captures.length;captures.push(children[j]);c+='if(!_e(_c['+ci+'],i,d+1,_t,_v))return false;';}else{budget[0]-=r2.length;c+='if(!(function(i,d,_t,_v){'+r2+'})(i,d+1,_t,_v))return false;';}} return c+'return true;'; }
    case 93: { var r=R('t'); if(!r)return null; var c=r+TO+'return true;if(!Object.hasOwn(t,'+JSON.stringify(value)+'))return true;'; if(children&&children.length>0)c+=seq(children,'i'); return c+'return true;'; }
    case 94: { var c=IO+'if(!Object.hasOwn(i,'+JSON.stringify(value)+'))return true;'; if(children&&children.length>0)c+=seq(children,'i'); return c+'return true;'; }
    case 95: { var c='if(_jt(i)!=='+value+')return true;'; if(children&&children.length>0)c+=seq(children,'i'); return c+'return true;'; }
    case 96: return 'return true;';
    case 97: return fb(97);
    case 98: { if(!value)return 'return true;'; if(visited&&visited.has(instruction))return fb(98); if(!visited)visited=new Set(); visited.add(instruction); var r=R('t'); if(!r)return fb(98); var c=r; for(var j=0;j<value.length;j++){var r2=compileInstructionToCode(value[j],captures,visited,budget); if(r2===null){var ci=captures.length;captures.push(value[j]);c+='if(!_e(_c['+ci+'],t,d+1,_t,_v))return false;';}else{budget[0]-=r2.length;c+='if(!(function(i,d,_t,_v){'+r2+'})(t,d+1,_t,_v))return false;';}} return c+'return true;'; }
    default: return null;
  }
}

function generateNativeValidator(template) {
  if (template[1] || template[2]) return null;
  const targets = template[3];
  if (targets.length === 0) return () => true;
  const instructions = targets[0];
  if (instructions.length === 0) return () => true;

  const capturedInstructions = [];
  const budget = [5000000];
  let body = '';
  for (let index = 0; index < instructions.length; index++) {
    const code = compileInstructionToCode(instructions[index], capturedInstructions, null, budget);
    if (code === null) return null;
    budget[0] -= code.length;
    body += `if(!(function(i,d,_t,_v){${code}})(instance,0,_t,_v))return false;`;
  }
  body += 'return true;';

  try {
    const fn = eval(
      '(function(_es,_jt,_ii,_ul,_e,_fh,_c,_t){' +
      'return function(instance,_v){' + body + '};' +
      '})'
    );
    return fn(
      effectiveTypeStrictReal, jsonTypeOf, isIntegral, unicodeLength,
      evaluateInstructionFast, fastHandlers, capturedInstructions, template
    );
  } catch {
    return null;
  }
}

function sourceToBigInt(source) {
  // Plain integer like "99999999999999999999999999999999999"
  if (/^-?[0-9]+$/.test(source)) return BigInt(source);
  // Exponential notation like "9.9999999999999999e+34". Parse the coefficient
  // and exponent to reconstruct the exact integer, as `BigInt` cannot parse
  // exponential notation directly
  const match = source.match(/^(-?)([0-9]+)\.?([0-9]*)[eE][+]?([0-9]+)$/);
  if (!match) return null;
  const exponent = parseInt(match[4]) - match[3].length;
  if (exponent < 0) return null;
  return BigInt(match[1] + match[2] + match[3]) * (10n ** BigInt(exponent));
}

function reviver(_key, value, context) {
  // On older engines, `JSON.parse` calls the reviver with only two arguments
  if (context === undefined) return value;
  if (typeof value !== 'number') return value;
  const source = context.source;
  // Only convert when `JSON.parse` actually truncated the value, avoiding
  // unnecessary `BigInt` conversion for integers that are exactly
  // representable as doubles
  if (String(value) === source) return value;
  const bigint = sourceToBigInt(source);
  return bigint !== null ? bigint : value;
}

class Blaze {
  static reviver = reviver;

  constructor(template) {
    if (!Array.isArray(template) || template[0] !== JSON_VERSION) {
      throw new Error(
        `Only version ${JSON_VERSION} of the compiled template is supported by this version of the evaluator`);
    }
    compile(template);
    this.template = template;
    this.callbackMode = false;
    this.trackMode = false;
    this.dynamicMode = false;
    this.callback = null;
    this.evaluatePathLength = 0;
    this.evaluatePathTokens = null;
    this.instanceLocationLength = 0;
    this.instanceLocationTokens = null;
    this.resources = null;
    this.evaluated = null;
    this.propertyTarget = undefined;
    this.propertyParent = undefined;
    this.propertyKey = undefined;
    this._nativeValidate = generateNativeValidator(template);
  }

  validate(instance, callback) {
    if (callback === undefined && this._nativeValidate) {
      return this._nativeValidate(instance, this);
    }

    const template = this.template;
    const targets = template[3];
    if (targets.length === 0) return true;

    const track = template[2];
    const dynamic = template[1];
    this.trackMode = track;
    this.dynamicMode = dynamic;
    this.callbackMode = callback !== undefined;
    this.callback = callback;

    if (this.callbackMode) {
      this.evaluatePathLength = 0;
      this.evaluatePathTokens = [];
      this.instanceLocationLength = 0;
      this.instanceLocationTokens = [];
      this.resources = [];
      if (track || dynamic) {
        evaluateInstruction = evaluateInstructionTrackedCallback;
        if (track) {
          this.evaluated = [];
        }
      } else {
        evaluateInstruction = evaluateInstructionFastCallback;
      }
    } else if (track || dynamic) {
      evaluateInstruction = evaluateInstructionTracked;
      if (track) {
        this.evaluatePathLength = 0;
        this.evaluatePathTokens = [];
        this.evaluated = [];
      }
      if (dynamic) {
        this.resources = [];
      }
    } else {
      evaluateInstruction = evaluateInstructionFast;
    }

    const instructions = targets[0];
    let result = true;
    for (let index = 0; index < instructions.length; index++) {
      if (!evaluateInstruction(instructions[index], instance, 0, template, this)) {
        result = false;
        break;
      }
    }

    this.resources = null;
    this.evaluated = null;
    this.evaluatePathTokens = null;
    this.instanceLocationTokens = null;
    this.callback = null;
    this.callbackMode = false;
    return result;
  }

  snapshotPathTokens() {
    return this.evaluatePathTokens.slice(0, this.evaluatePathLength);
  }

  pushPath(relativeSchemaLocation) {
    for (let index = 0; index < relativeSchemaLocation.length; index++) {
      if (this.evaluatePathLength < this.evaluatePathTokens.length) {
        this.evaluatePathTokens[this.evaluatePathLength] = relativeSchemaLocation[index];
      } else {
        this.evaluatePathTokens.push(relativeSchemaLocation[index]);
      }
      this.evaluatePathLength++;
    }
  }

  popPath(count) {
    this.evaluatePathLength -= count;
  }

  callbackPush(instruction) {
    if (!this.trackMode) {
      this.pushPath(instruction[1]);
    }
    const relInstance = instruction[2];
    for (let index = 0; index < relInstance.length; index++) {
      this.pushInstanceToken(relInstance[index]);
    }
    this.callback("pre", true, instruction,
      buildJsonPointer(this.evaluatePathTokens, this.evaluatePathLength),
      buildJsonPointer(this.instanceLocationTokens, this.instanceLocationLength),
      null);
  }

  callbackPop(instruction, result) {
    const isAnnotation = instruction[0] >= ANNOTATION_EMIT &&
                         instruction[0] <= ANNOTATION_BASENAME_TO_PARENT;
    this.callback("post", result, instruction,
      buildJsonPointer(this.evaluatePathTokens, this.evaluatePathLength),
      buildJsonPointer(this.instanceLocationTokens, this.instanceLocationLength),
      isAnnotation ? instruction[5] : null);
    if (!this.trackMode) {
      this.popPath(instruction[1].length);
    }
    const relInstance = instruction[2];
    for (let index = 0; index < relInstance.length; index++) {
      this.popInstanceToken();
    }
  }

  callbackAnnotation(instruction) {
    if (!this.trackMode) {
      this.pushPath(instruction[1]);
    }
    const relInstance = instruction[2];
    for (let index = 0; index < relInstance.length; index++) {
      this.pushInstanceToken(relInstance[index]);
    }
    const evaluatePath = buildJsonPointer(this.evaluatePathTokens, this.evaluatePathLength);
    const opcode = instruction[0];
    let instanceLocation;
    if (opcode === ANNOTATION_EMIT) {
      instanceLocation = buildJsonPointer(this.instanceLocationTokens, this.instanceLocationLength);
    } else {
      const parentLength = this.instanceLocationLength > 0 ? this.instanceLocationLength - 1 : 0;
      instanceLocation = buildJsonPointer(this.instanceLocationTokens, parentLength);
    }
    let annotationValue = instruction[5];
    if (opcode === ANNOTATION_BASENAME_TO_PARENT && this.instanceLocationLength > 0) {
      annotationValue = this.instanceLocationTokens[this.instanceLocationLength - 1];
    }
    this.callback("pre", true, instruction, evaluatePath, instanceLocation, null);
    this.callback("post", true, instruction, evaluatePath, instanceLocation, annotationValue);
    if (!this.trackMode) {
      this.popPath(instruction[1].length);
    }
    for (let index = 0; index < relInstance.length; index++) {
      this.popInstanceToken();
    }
  }

  pushInstanceToken(token) {
    if (this.instanceLocationLength < this.instanceLocationTokens.length) {
      this.instanceLocationTokens[this.instanceLocationLength] = token;
    } else {
      this.instanceLocationTokens.push(token);
    }
    this.instanceLocationLength++;
  }

  popInstanceToken() {
    this.instanceLocationLength--;
  }

  markEvaluated(target, parent, key) {
    this.evaluated.push({
      instance: target,
      parent: parent,
      key: key,
      pathTokens: this.snapshotPathTokens(),
      pathLength: this.evaluatePathLength,
      skip: false
    });
  }

  isEvaluated(target, parent, key) {
    const pathLen = this.evaluatePathLength;
    const initialLen = pathLen <= 1 ? 0 : pathLen - 1;
    const initialTokens = this.evaluatePathTokens;
    const isPrimitive = target === null || typeof target !== 'object';
    const hasLocation = parent !== undefined;

    for (let index = this.evaluated.length - 1; index >= 0; index--) {
      const entry = this.evaluated[index];
      if (entry.skip) continue;

      if (isPrimitive && hasLocation && entry.parent !== undefined) {
        if (entry.parent !== parent || entry.key !== key) continue;
      } else {
        if (entry.instance !== target) continue;
      }

      if (initialLen === 0) return true;
      if (entry.pathLength < initialLen) continue;
      let match = true;
      for (let token = 0; token < initialLen; token++) {
        if (entry.pathTokens[token] !== initialTokens[token]) {
          match = false;
          break;
        }
      }
      if (match) return true;
    }
    return false;
  }

  unevaluate() {
    const pathLen = this.evaluatePathLength;
    const tokens = this.evaluatePathTokens;
    for (let index = 0; index < this.evaluated.length; index++) {
      const entry = this.evaluated[index];
      if (entry.skip) continue;
      if (entry.pathLength < pathLen) continue;
      let match = true;
      for (let token = 0; token < pathLen; token++) {
        if (entry.pathTokens[token] !== tokens[token]) {
          match = false;
          break;
        }
      }
      if (match) entry.skip = true;
    }
  }
}

function evaluateInstructionFast(instruction, instance, depth, template, evaluator) {
  if (depth > DEPTH_LIMIT) {
    throw new Error('The evaluation path depth limit was reached likely due to infinite recursion');
  }
  const handler = fastHandlers[instruction[0]];
  if (!handler) return true;
  return handler(instruction, instance, depth, template, evaluator);
}

function evaluateInstructionTracked(instruction, instance, depth, template, evaluator) {
  if (depth > DEPTH_LIMIT) {
    throw new Error('The evaluation path depth limit was reached likely due to infinite recursion');
  }
  const handler = fastHandlers[instruction[0]];
  if (!handler) return true;

  const type = instruction[0];
  if (type < 92 || type > 96) {
    if (evaluator.trackMode) {
      evaluator.pushPath(instruction[1]);
    }
    if (evaluator.dynamicMode) {
      evaluator.resources.push(instruction[4]);
    }

    const result = handler(instruction, instance, depth, template, evaluator);

    if (evaluator.trackMode) {
      evaluator.popPath(instruction[1].length);
    }
    if (evaluator.dynamicMode) {
      evaluator.resources.pop();
    }
    return result;
  }

  return handler(instruction, instance, depth, template, evaluator);
}

function evaluateInstructionFastCallback(instruction, instance, depth, template, evaluator) {
  if (depth > DEPTH_LIMIT) {
    throw new Error('The evaluation path depth limit was reached likely due to infinite recursion');
  }
  const handler = handlers[instruction[0]];
  if (!handler) return true;
  return handler(instruction, instance, depth, template, evaluator);
}

function evaluateInstructionTrackedCallback(instruction, instance, depth, template, evaluator) {
  if (depth > DEPTH_LIMIT) {
    throw new Error('The evaluation path depth limit was reached likely due to infinite recursion');
  }
  const handler = handlers[instruction[0]];
  if (!handler) return true;

  const type = instruction[0];
  if (type < CONTROL_GROUP_START || type > CONTROL_EVALUATE_END) {
    if (evaluator.trackMode) {
      evaluator.pushPath(instruction[1]);
    }
    if (evaluator.dynamicMode) {
      evaluator.resources.push(instruction[4]);
    }

    const result = handler(instruction, instance, depth, template, evaluator);

    if (evaluator.trackMode) {
      evaluator.popPath(instruction[1].length);
    }
    if (evaluator.dynamicMode) {
      evaluator.resources.pop();
    }
    return result;
  }

  return handler(instruction, instance, depth, template, evaluator);
}

let evaluateInstruction = evaluateInstructionFast;

function effectiveTypeStrictReal(value) {
  if (value === null) return Type.Null;
  switch (typeof value) {
    case 'boolean': return Type.Boolean;
    case 'number': return Number.isInteger(value) ? Type.Integer : Type.Real;
    case 'bigint': return Type.Integer;
    case 'string': return Type.String;
    case 'object': return Array.isArray(value) ? Type.Array : Type.Object;
    default: return Type.Null;
  }
}

function typeSetTest(bitmask, typeIndex) {
  return (bitmask & (1 << typeIndex)) !== 0;
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
  const keysLeft = Object.keys(left);
  const keysRight = Object.keys(right);
  if (keysLeft.length !== keysRight.length) return false;
  for (let index = 0; index < keysLeft.length; index++) {
    const key = keysLeft[index];
    if (!Object.hasOwn(right, key)) return false;
    if (!jsonEqual(left[key], right[key])) return false;
  }
  return true;
}

function fastHash(value) {
  if (value === null) return 2;
  switch (typeof value) {
    case 'boolean': return value ? 1 : 0;
    case 'number': return 4 + ((value | 0) & 255);
    case 'bigint': return 4 + Number(value & 255n);
    case 'string': return 3 + value.length;
    case 'object':
      if (Array.isArray(value)) {
        let hash = 6;
        for (let index = 0; index < value.length; index++) hash += 1 + fastHash(value[index]);
        return hash;
      } else {
        let hash = 7;
        for (const key in value) hash += 1 + key.length + fastHash(value[key]);
        return hash;
      }
    default: return 2;
  }
}

function isUnique(array) {
  const length = array.length;
  if (length <= 1) return true;
  const first = array[0];
  const firstType = typeof first;
  if (first !== null && firstType !== 'object') {
    let allPrimitive = true;
    for (let index = 1; index < length; index++) {
      const element = array[index];
      if (element === null || typeof element === 'object') {
        allPrimitive = false;
        break;
      }
    }
    if (allPrimitive) {
      const set = new Set();
      for (let index = 0; index < length; index++) {
        const size = set.size;
        set.add(array[index]);
        if (set.size === size) return false;
      }
      return true;
    }
  }
  const hashes = new Array(length);
  for (let index = 0; index < length; index++) {
    hashes[index] = fastHash(array[index]);
  }
  for (let index = 1; index < length; index++) {
    for (let previous = 0; previous < index; previous++) {
      if (hashes[index] === hashes[previous] && jsonEqual(array[index], array[previous])) return false;
    }
  }
  return true;
}

function isDivisibleBy(value, divisor) {
  if (divisor === 0 || divisor === 0n) return false;
  if (typeof value === 'bigint' && typeof divisor === 'bigint') {
    return value % divisor === 0n;
  }
  if (typeof value === 'bigint') {
    value = Number(value);
  } else if (typeof divisor === 'bigint') {
    divisor = Number(divisor);
  }
  const remainder = value % divisor;
  if (remainder === 0) return true;
  return Math.abs(remainder) < 1e-9 || Math.abs(remainder - divisor) < 1e-9 ||
         Math.abs(remainder + divisor) < 1e-9;
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

function objectSize(object) {
  let count = 0;
  for (const key in object) count++;
  return count;
}

function isObject(value) {
  return value !== null && typeof value === 'object' && !Array.isArray(value);
}

function AssertionFail(instruction, instance, depth, template, evaluator) {
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
  return false;
};

function AssertionDefines(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const __result = Object.hasOwn(target, instruction[5]);
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, __result);
  return __result;
};

function AssertionDefinesStrict(instruction, instance, depth, template, evaluator) {
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) {
    if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
    return false;
  }
  const __result = Object.hasOwn(target, instruction[5]);
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, __result);
  return __result;
};

function AssertionDefinesAll(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const strings = instruction[5];
  for (let index = 0; index < strings.length; index++) {
    if (!Object.hasOwn(target, strings[index])) {
      if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
      return false;
    }
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function AssertionDefinesAllStrict(instruction, instance, depth, template, evaluator) {
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) {
    if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
    return false;
  }
  const strings = instruction[5];
  for (let index = 0; index < strings.length; index++) {
    if (!Object.hasOwn(target, strings[index])) {
      if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
      return false;
    }
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function AssertionDefinesExactly(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  let targetSize = 0;
  for (const key in target) targetSize++;
  const strings = instruction[5];
  if (targetSize !== strings.length) {
    if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
    return false;
  }
  for (let index = 0; index < strings.length; index++) {
    if (!Object.hasOwn(target, strings[index])) {
      if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
      return false;
    }
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function AssertionDefinesExactlyStrict(instruction, instance, depth, template, evaluator) {
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) {
    if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
    return false;
  }
  let targetSize = 0;
  for (const key in target) targetSize++;
  const strings = instruction[5];
  if (targetSize !== strings.length) {
    if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
    return false;
  }
  for (let index = 0; index < strings.length; index++) {
    if (!Object.hasOwn(target, strings[index])) {
      if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
      return false;
    }
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function AssertionDefinesExactlyStrictHash3(instruction, instance, depth, template, evaluator) {
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) {
    if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
    return false;
  }
  const entries = instruction[5][0];
  let count = 0;
  for (const key in target) count++;
  if (count !== 3) {
    if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
    return false;
  }
  const __result = Object.hasOwn(target, entries[0][1]) &&
    Object.hasOwn(target, entries[1][1]) &&
    Object.hasOwn(target, entries[2][1]);
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, __result);
  return __result;
};

function AssertionPropertyDependencies(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const value = instruction[5];
  for (const property in value) {
    if (!Object.hasOwn(target, property)) continue;
    const dependencies = value[property];
    for (let index = 0; index < dependencies.length; index++) {
      if (!Object.hasOwn(target, dependencies[index])) {
        if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
        return false;
      }
    }
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function AssertionType(instruction, instance, depth, template, evaluator) {
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const target = resolveInstance(instance, instruction[2]);
  const expected = instruction[5];
  const actual = jsonTypeOf(target);
  if (actual === expected) {
    if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
    return true;
  }
  if (expected === Type.Integer && isIntegral(target)) {
    if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
    return true;
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
  return false;
};

function AssertionTypeAny(instruction, instance, depth, template, evaluator) {
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const target = resolveInstance(instance, instruction[2]);
  const bitmask = instruction[5];
  const typeIndex = jsonTypeOf(target);
  if (typeSetTest(bitmask, typeIndex)) {
    if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
    return true;
  }
  if (typeSetTest(bitmask, Type.Integer) && isIntegral(target)) {
    if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
    return true;
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
  return false;
};

function AssertionTypeStrict(instruction, instance, depth, template, evaluator) {
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const target = resolveInstance(instance, instruction[2]);
  const __result = effectiveTypeStrictReal(target) === instruction[5];
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, __result);
  return __result;
};

function AssertionTypeStrictAny(instruction, instance, depth, template, evaluator) {
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const target = resolveInstance(instance, instruction[2]);
  const __result = typeSetTest(instruction[5], effectiveTypeStrictReal(target));
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, __result);
  return __result;
};

function AssertionTypeStringBounded(instruction, instance, depth, template, evaluator) {
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const target = resolveInstance(instance, instruction[2]);
  if (typeof target !== 'string') {
    if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
    return false;
  }
  const range = instruction[5];
  const length = unicodeLength(target);
  if (length < range[0]) {
    if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
    return false;
  }
  if (range[1] !== null && length > range[1]) {
    if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
    return false;
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function AssertionTypeStringUpper(instruction, instance, depth, template, evaluator) {
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const target = resolveInstance(instance, instruction[2]);
  const __result = typeof target === 'string' && unicodeLength(target) <= instruction[5];
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, __result);
  return __result;
};

function AssertionTypeArrayBounded(instruction, instance, depth, template, evaluator) {
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const target = resolveInstance(instance, instruction[2]);
  if (!Array.isArray(target)) {
    if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
    return false;
  }
  const range = instruction[5];
  if (target.length < range[0]) {
    if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
    return false;
  }
  if (range[1] !== null && target.length > range[1]) {
    if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
    return false;
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function AssertionTypeArrayUpper(instruction, instance, depth, template, evaluator) {
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const target = resolveInstance(instance, instruction[2]);
  const __result = Array.isArray(target) && target.length <= instruction[5];
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, __result);
  return __result;
};

function AssertionTypeObjectBounded(instruction, instance, depth, template, evaluator) {
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) {
    if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
    return false;
  }
  const range = instruction[5];
  const size = objectSize(target);
  if (size < range[0]) {
    if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
    return false;
  }
  if (range[1] !== null && size > range[1]) {
    if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
    return false;
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function AssertionTypeObjectUpper(instruction, instance, depth, template, evaluator) {
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) {
    if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
    return false;
  }
  const __result = objectSize(target) <= instruction[5];
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, __result);
  return __result;
};

function AssertionRegex(instruction, instance, depth, template, evaluator) {
  const target = evaluator.propertyTarget !== undefined
    ? evaluator.propertyTarget : resolveInstance(instance, instruction[2]);
  if (typeof target !== 'string') return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const __result = instruction[5].test(target);
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, __result);
  return __result;
};

function AssertionStringSizeLess(instruction, instance, depth, template, evaluator) {
  const target = evaluator.propertyTarget !== undefined
    ? evaluator.propertyTarget : resolveInstance(instance, instruction[2]);
  if (typeof target !== 'string') return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const __result = unicodeLength(target) < instruction[5];
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, __result);
  return __result;
};

function AssertionStringSizeGreater(instruction, instance, depth, template, evaluator) {
  const target = evaluator.propertyTarget !== undefined
    ? evaluator.propertyTarget : resolveInstance(instance, instruction[2]);
  if (typeof target !== 'string') return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const __result = unicodeLength(target) > instruction[5];
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, __result);
  return __result;
};

function AssertionArraySizeLess(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!Array.isArray(target)) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const __result = target.length < instruction[5];
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, __result);
  return __result;
};

function AssertionArraySizeGreater(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!Array.isArray(target)) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const __result = target.length > instruction[5];
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, __result);
  return __result;
};

function AssertionObjectSizeLess(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const __result = objectSize(target) < instruction[5];
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, __result);
  return __result;
};

function AssertionObjectSizeGreater(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const __result = objectSize(target) > instruction[5];
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, __result);
  return __result;
};

function AssertionEqual(instruction, instance, depth, template, evaluator) {
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  let __result;
  if (evaluator.propertyTarget !== undefined) {
    const value = instruction[5];
    __result = typeof value === 'string' && value === evaluator.propertyTarget;
  } else {
    __result = jsonEqual(resolveInstance(instance, instruction[2]), instruction[5]);
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, __result);
  return __result;
};

function AssertionEqualsAny(instruction, instance, depth, template, evaluator) {
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const value = instruction[5];
  const target = evaluator.propertyTarget !== undefined
    ? evaluator.propertyTarget : resolveInstance(instance, instruction[2]);
  if (value.primitive) {
    const __result = value.set.has(target);
    if (evaluator.callbackMode) evaluator.callbackPop(instruction, __result);
    return __result;
  }
  const values = Array.isArray(value) ? value : value.values;
  for (let index = 0; index < values.length; index++) {
    if (jsonEqual(target, values[index])) {
      if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
      return true;
    }
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
  return false;
};

function AssertionEqualsAnyStringHash(instruction, instance, depth, template, evaluator) {
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const target = evaluator.propertyTarget !== undefined
    ? evaluator.propertyTarget
    : resolveInstance(instance, instruction[2]);
  if (typeof target !== 'string') {
    if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
    return false;
  }

  const value = instruction[5];
  const entries = value[0];
  const tableOfContents = value[1];

  const stringSize = target.length;
  if (stringSize < tableOfContents.length) {
    const hint = tableOfContents[stringSize];
    if (hint[1] === 0) {
      if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
      return false;
    }
    for (let index = hint[0] - 1; index < hint[1]; index++) {
      if (entries[index][1] === target) {
        if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
        return true;
      }
    }
    if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
    return false;
  }

  if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
  return false;
};

function AssertionGreaterEqual(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  const targetType = typeof target;
  if (targetType !== 'number' && targetType !== 'bigint') return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const __result = target >= instruction[5];
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, __result);
  return __result;
};

function AssertionLessEqual(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  const targetType = typeof target;
  if (targetType !== 'number' && targetType !== 'bigint') return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const __result = target <= instruction[5];
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, __result);
  return __result;
};

function AssertionGreater(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  const targetType = typeof target;
  if (targetType !== 'number' && targetType !== 'bigint') return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const __result = target > instruction[5];
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, __result);
  return __result;
};

function AssertionLess(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  const targetType = typeof target;
  if (targetType !== 'number' && targetType !== 'bigint') return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const __result = target < instruction[5];
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, __result);
  return __result;
};

function AssertionUnique(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!Array.isArray(target)) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const __result = isUnique(target);
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, __result);
  return __result;
};

function AssertionDivisible(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  const targetType = typeof target;
  if (targetType !== 'number' && targetType !== 'bigint') return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const __result = isDivisibleBy(target, instruction[5]);
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, __result);
  return __result;
};

function AssertionTypeIntegerBounded(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const range = instruction[5];
  const __result = (typeof target === 'bigint' || Number.isInteger(target)) && target >= range[0] && target <= range[1];
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, __result);
  return __result;
};

function AssertionTypeIntegerBoundedStrict(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const range = instruction[5];
  const __result = (typeof target === 'bigint' || Number.isInteger(target)) && target >= range[0] && target <= range[1];
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, __result);
  return __result;
};

function AssertionTypeIntegerLowerBound(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const range = instruction[5];
  const __result = (typeof target === 'bigint' || Number.isInteger(target)) && target >= range[0];
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, __result);
  return __result;
};

function AssertionTypeIntegerLowerBoundStrict(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const range = instruction[5];
  const __result = (typeof target === 'bigint' || Number.isInteger(target)) && target >= range[0];
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, __result);
  return __result;
};

function AssertionStringType(instruction, instance, depth, template, evaluator) {
  const target = evaluator.propertyTarget !== undefined
    ? evaluator.propertyTarget : resolveInstance(instance, instruction[2]);
  if (typeof target !== 'string') return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const __result = URI_REGEX.test(target);
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, __result);
  return __result;
};

function AssertionPropertyType(instruction, instance, depth, template, evaluator) {
  if (!isObject(instance)) return true;
  const target = resolveInstance(instance, instruction[2]);
  if (target === undefined) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const expected = instruction[5];
  const actual = jsonTypeOf(target);
  const __result = actual === expected || (expected === Type.Integer && isIntegral(target));
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, __result);
  return __result;
};

function AssertionPropertyTypeEvaluate(instruction, instance, depth, template, evaluator) {
  if (!isObject(instance)) return true;
  const target = resolveInstance(instance, instruction[2]);
  if (target === undefined) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const expected = instruction[5];
  const actual = jsonTypeOf(target);
  const result = actual === expected || (expected === Type.Integer && isIntegral(target));
  if (result && evaluator.trackMode) {
    const location = instruction[2];
    evaluator.markEvaluated(target, instance, location.length > 0 ? location[location.length - 1] : undefined);
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, result);
  return result;
};

function AssertionPropertyTypeStrict(instruction, instance, depth, template, evaluator) {
  if (!isObject(instance)) return true;
  const target = resolveInstance(instance, instruction[2]);
  if (target === undefined) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const __result = effectiveTypeStrictReal(target) === instruction[5];
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, __result);
  return __result;
};

function AssertionPropertyTypeStrictEvaluate(instruction, instance, depth, template, evaluator) {
  if (!isObject(instance)) return true;
  const target = resolveInstance(instance, instruction[2]);
  if (target === undefined) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const result = effectiveTypeStrictReal(target) === instruction[5];
  if (result && evaluator.trackMode) {
    const location = instruction[2];
    evaluator.markEvaluated(target, instance, location.length > 0 ? location[location.length - 1] : undefined);
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, result);
  return result;
};

function AssertionPropertyTypeStrictAny(instruction, instance, depth, template, evaluator) {
  if (!isObject(instance)) return true;
  const target = resolveInstance(instance, instruction[2]);
  if (target === undefined) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const __result = typeSetTest(instruction[5], effectiveTypeStrictReal(target));
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, __result);
  return __result;
};

function AssertionPropertyTypeStrictAnyEvaluate(instruction, instance, depth, template, evaluator) {
  if (!isObject(instance)) return true;
  const target = resolveInstance(instance, instruction[2]);
  if (target === undefined) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const result = typeSetTest(instruction[5], effectiveTypeStrictReal(target));
  if (result && evaluator.trackMode) {
    const location = instruction[2];
    evaluator.markEvaluated(target, instance, location.length > 0 ? location[location.length - 1] : undefined);
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, result);
  return result;
};

function AssertionArrayPrefix(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!Array.isArray(target)) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  if (target.length === 0) {
    if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
    return true;
  }
  const children = instruction[6];
  const prefixes = children.length - 1;
  const pointer = target.length === prefixes ? prefixes : Math.min(target.length, prefixes) - 1;
  const entry = children[pointer];
  const entryChildren = entry[6];
  if (entryChildren) {
    for (let index = 0; index < entryChildren.length; index++) {
      if (!evaluateInstruction(entryChildren[index], target, depth + 1, template, evaluator)) {
        if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
        return false;
      }
    }
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function AssertionArrayPrefixEvaluate(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!Array.isArray(target)) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  if (target.length === 0) {
    if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
    return true;
  }
  const children = instruction[6];
  const prefixes = children.length - 1;
  const pointer = target.length === prefixes ? prefixes : Math.min(target.length, prefixes) - 1;
  const entry = children[pointer];
  const entryChildren = entry[6];
  if (entryChildren) {
    for (let index = 0; index < entryChildren.length; index++) {
      if (!evaluateInstruction(entryChildren[index], target, depth + 1, template, evaluator)) {
        if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
        return false;
      }
    }
  }
  if (evaluator.trackMode) {
    if (target.length === prefixes) {
      evaluator.markEvaluated(target);
    } else {
      for (let cursor = 0; cursor <= pointer; cursor++) {
        evaluator.markEvaluated(target[cursor], target, cursor);
      }
    }
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function AssertionObjectPropertiesSimple(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  if (!isObject(target)) {
    if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
    return false;
  }
  const value = instruction[5];
  const children = instruction[6];
  for (let index = 0; index < value.length; index++) {
    const entry = value[index];
    const name = entry[0];
    const required = entry[2];
    if (!Object.hasOwn(target, name)) {
      if (required) {
        if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
        return false;
      }
      continue;
    }
    if (index < children.length) {
      if (!evaluateInstructionFast(children[index], target[name], depth + 1, template, evaluator)) {
        if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
        return false;
      }
    }
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function AnnotationEmit(instruction, instance, depth, template, evaluator) {
  if (evaluator.callbackMode) evaluator.callbackAnnotation(instruction);
  return true;
}
function AnnotationToParent(instruction, instance, depth, template, evaluator) {
  if (evaluator.callbackMode) evaluator.callbackAnnotation(instruction);
  return true;
}
function AnnotationBasenameToParent(instruction, instance, depth, template, evaluator) {
  if (evaluator.callbackMode) evaluator.callbackAnnotation(instruction);
  return true;
}

function Evaluate(instruction, instance, depth, template, evaluator) {
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  if (evaluator.trackMode) {
    const target = resolveInstance(instance, instruction[2]);
    evaluator.markEvaluated(target);
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function LogicalNot(instruction, instance, depth, template, evaluator) {
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const target = resolveInstance(instance, instruction[2]);
  const children = instruction[6];
  if (children) {
    for (let index = 0; index < children.length; index++) {
      if (!evaluateInstruction(children[index], target, depth + 1, template, evaluator)) {
        if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
        return true;
      }
    }
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
  return false;
};

function LogicalNotEvaluate(instruction, instance, depth, template, evaluator) {
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const target = resolveInstance(instance, instruction[2]);
  const children = instruction[6];
  let result = false;
  if (children) {
    for (let index = 0; index < children.length; index++) {
      if (!evaluateInstruction(children[index], target, depth + 1, template, evaluator)) {
        result = true;
        break;
      }
    }
  }
  if (evaluator.trackMode) evaluator.unevaluate();
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, result);
  return result;
};

function LogicalOr(instruction, instance, depth, template, evaluator) {
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const children = instruction[6];
  if (!children || children.length === 0) {
    if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
    return true;
  }
  const target = resolveInstance(instance, instruction[2]);
  const exhaustive = instruction[5];
  let result = false;
  if (exhaustive) {
    for (let index = 0; index < children.length; index++) {
      if (evaluateInstruction(children[index], target, depth + 1, template, evaluator)) {
        result = true;
      }
    }
  } else {
    for (let index = 0; index < children.length; index++) {
      if (evaluateInstruction(children[index], target, depth + 1, template, evaluator)) {
        result = true;
        break;
      }
    }
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, result);
  return result;
};

function LogicalAnd(instruction, instance, depth, template, evaluator) {
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const target = resolveInstance(instance, instruction[2]);
  const children = instruction[6];
  if (children) {
    for (let index = 0; index < children.length; index++) {
      if (!evaluateInstruction(children[index], target, depth + 1, template, evaluator)) {
        if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
        return false;
      }
    }
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function LogicalXor(instruction, instance, depth, template, evaluator) {
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const target = resolveInstance(instance, instruction[2]);
  const exhaustive = instruction[5];
  const children = instruction[6];
  let result = true;
  let hasMatched = false;
  if (children) {
    for (let index = 0; index < children.length; index++) {
      if (evaluateInstruction(children[index], target, depth + 1, template, evaluator)) {
        if (hasMatched) {
          result = false;
          if (!exhaustive) break;
        } else {
          hasMatched = true;
        }
      }
    }
  }
  const __result = result && hasMatched;
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, __result);
  return __result;
};

function LogicalCondition(instruction, instance, depth, template, evaluator) {
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const value = instruction[5];
  const thenStart = value[0];
  const elseStart = value[1];
  const children = instruction[6];
  const childrenSize = children ? children.length : 0;

  let conditionEnd = childrenSize;
  if (thenStart > 0) conditionEnd = thenStart;
  else if (elseStart > 0) conditionEnd = elseStart;

  const target = resolveInstance(instance, instruction[2]);

  let conditionResult = true;
  for (let cursor = 0; cursor < conditionEnd; cursor++) {
    if (!evaluateInstruction(children[cursor], target, depth + 1, template, evaluator)) {
      conditionResult = false;
      break;
    }
  }

  const consequenceStart = conditionResult ? thenStart : elseStart;
  const consequenceEnd = (conditionResult && elseStart > 0) ? elseStart : childrenSize;

  if (consequenceStart > 0) {
    if (evaluator.trackMode || evaluator.callbackMode) {
      evaluator.popPath(instruction[1].length);
    }

    let result = true;
    for (let cursor = consequenceStart; cursor < consequenceEnd; cursor++) {
      if (!evaluateInstruction(children[cursor], target, depth + 1, template, evaluator)) {
        result = false;
        break;
      }
    }

    if (evaluator.trackMode || evaluator.callbackMode) {
      evaluator.pushPath(instruction[1]);
    }

    if (evaluator.callbackMode) evaluator.callbackPop(instruction, result);
    return result;
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function LogicalWhenType(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (jsonTypeOf(target) !== instruction[5]) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const children = instruction[6];
  if (children) {
    for (let index = 0; index < children.length; index++) {
      if (!evaluateInstruction(children[index], target, depth + 1, template, evaluator)) {
        if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
        return false;
      }
    }
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function LogicalWhenDefines(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return true;
  if (!Object.hasOwn(target, instruction[5])) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const children = instruction[6];
  if (children) {
    for (let index = 0; index < children.length; index++) {
      if (!evaluateInstruction(children[index], target, depth + 1, template, evaluator)) {
        if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
        return false;
      }
    }
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function LogicalWhenArraySizeGreater(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!Array.isArray(target) || target.length <= instruction[5]) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const children = instruction[6];
  if (children) {
    for (let index = 0; index < children.length; index++) {
      if (!evaluateInstruction(children[index], target, depth + 1, template, evaluator)) {
        if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
        return false;
      }
    }
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function LoopPropertiesUnevaluated(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  if (evaluator.trackMode && evaluator.isEvaluated(target)) {
    if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
    return true;
  }
  const children = instruction[6];
  for (const key in target) {
    if (evaluator.trackMode && evaluator.isEvaluated(target[key], target, key)) continue;
    if (evaluator.callbackMode) evaluator.pushInstanceToken(key);
    if (children) {
      for (let childIndex = 0; childIndex < children.length; childIndex++) {
        if (!evaluateInstruction(children[childIndex], target[key], depth + 1, template, evaluator)) {
          if (evaluator.callbackMode) evaluator.popInstanceToken();
          if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
          return false;
        }
      }
    }
    if (evaluator.callbackMode) evaluator.popInstanceToken();
  }
  if (evaluator.trackMode) evaluator.markEvaluated(target);
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function LoopPropertiesUnevaluatedExcept(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  if (evaluator.trackMode && evaluator.isEvaluated(target)) {
    if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
    return true;
  }
  const filter = instruction[5];
  const filterStrings = filter[0];
  const filterPrefixes = filter[1];
  const filterRegexes = filter[2];
  const children = instruction[6];
  for (const key in target) {
    if (filterStrings.has(key)) continue;
    let matched = false;
    for (let index = 0; index < filterPrefixes.length; index++) {
      if (key.startsWith(filterPrefixes[index])) { matched = true; break; }
    }
    if (matched) continue;
    for (let index = 0; index < filterRegexes.length; index++) {
      filterRegexes[index].lastIndex = 0;
      if (filterRegexes[index].test(key)) { matched = true; break; }
    }
    if (matched) continue;
    if (evaluator.trackMode && evaluator.isEvaluated(target[key], target, key)) continue;
    if (evaluator.callbackMode) evaluator.pushInstanceToken(key);
    if (children) {
      for (let childIndex = 0; childIndex < children.length; childIndex++) {
        if (!evaluateInstruction(children[childIndex], target[key], depth + 1, template, evaluator)) {
          if (evaluator.callbackMode) evaluator.popInstanceToken();
          if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
          return false;
        }
      }
    }
    if (evaluator.callbackMode) evaluator.popInstanceToken();
  }
  if (evaluator.trackMode) evaluator.markEvaluated(target);
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function LoopPropertiesMatch(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const children = instruction[6];
  for (const key in target) {
    const index = instruction[5][key];
    if (index === undefined) continue;
    const subinstruction = children[index];
    const subchildren = subinstruction[6];
    if (subchildren) {
      for (let childIndex = 0; childIndex < subchildren.length; childIndex++) {
        if (!evaluateInstruction(subchildren[childIndex], target, depth + 1, template, evaluator)) {
          if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
          return false;
        }
      }
    }
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function LoopPropertiesMatchClosed(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const children = instruction[6];
  for (const key in target) {
    const index = instruction[5][key];
    if (index === undefined) {
      if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
      return false;
    }
    const subinstruction = children[index];
    const subchildren = subinstruction[6];
    if (subchildren) {
      for (let childIndex = 0; childIndex < subchildren.length; childIndex++) {
        if (!evaluateInstruction(subchildren[childIndex], target, depth + 1, template, evaluator)) {
          if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
          return false;
        }
      }
    }
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function LoopProperties(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const children = instruction[6];
  for (const key in target) {
    evaluator.propertyParent = target;
    evaluator.propertyKey = key;
    if (evaluator.callbackMode) evaluator.pushInstanceToken(key);
    if (children) {
      for (let childIndex = 0; childIndex < children.length; childIndex++) {
        if (!evaluateInstruction(children[childIndex], target[key], depth + 1, template, evaluator)) {
          if (evaluator.callbackMode) evaluator.popInstanceToken();
          if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
          return false;
        }
      }
    }
    if (evaluator.callbackMode) evaluator.popInstanceToken();
  }
  evaluator.propertyParent = undefined;
  evaluator.propertyKey = undefined;
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function LoopPropertiesEvaluate(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const children = instruction[6];
  for (const key in target) {
    evaluator.propertyParent = target;
    evaluator.propertyKey = key;
    if (evaluator.callbackMode) evaluator.pushInstanceToken(key);
    if (children) {
      for (let childIndex = 0; childIndex < children.length; childIndex++) {
        if (!evaluateInstruction(children[childIndex], target[key], depth + 1, template, evaluator)) {
          if (evaluator.callbackMode) evaluator.popInstanceToken();
          if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
          return false;
        }
      }
    }
    if (evaluator.callbackMode) evaluator.popInstanceToken();
  }
  evaluator.propertyParent = undefined;
  evaluator.propertyKey = undefined;
  if (evaluator.trackMode) evaluator.markEvaluated(target);
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function LoopPropertiesRegex(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const regex = instruction[5];
  const children = instruction[6];
  for (const key in target) {
    regex.lastIndex = 0;
    if (!regex.test(key)) continue;
    evaluator.propertyParent = target;
    evaluator.propertyKey = key;
    if (evaluator.callbackMode) evaluator.pushInstanceToken(key);
    if (children) {
      for (let childIndex = 0; childIndex < children.length; childIndex++) {
        if (!evaluateInstruction(children[childIndex], target[key], depth + 1, template, evaluator)) {
          if (evaluator.callbackMode) evaluator.popInstanceToken();
          if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
          return false;
        }
      }
    }
    if (evaluator.callbackMode) evaluator.popInstanceToken();
  }
  evaluator.propertyParent = undefined;
  evaluator.propertyKey = undefined;
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function LoopPropertiesRegexClosed(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const regex = instruction[5];
  const children = instruction[6];
  for (const key in target) {
    regex.lastIndex = 0;
    if (!regex.test(key)) {
      if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
      return false;
    }
    evaluator.propertyParent = target;
    evaluator.propertyKey = key;
    if (evaluator.callbackMode) evaluator.pushInstanceToken(key);
    if (children) {
      for (let childIndex = 0; childIndex < children.length; childIndex++) {
        if (!evaluateInstruction(children[childIndex], target[key], depth + 1, template, evaluator)) {
          if (evaluator.callbackMode) evaluator.popInstanceToken();
          if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
          return false;
        }
      }
    }
    if (evaluator.callbackMode) evaluator.popInstanceToken();
  }
  evaluator.propertyParent = undefined;
  evaluator.propertyKey = undefined;
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function LoopPropertiesStartsWith(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const prefix = instruction[5];
  const children = instruction[6];
  for (const key in target) {
    if (!key.startsWith(prefix)) continue;
    evaluator.propertyParent = target;
    evaluator.propertyKey = key;
    if (evaluator.callbackMode) evaluator.pushInstanceToken(key);
    if (children) {
      for (let childIndex = 0; childIndex < children.length; childIndex++) {
        if (!evaluateInstruction(children[childIndex], target[key], depth + 1, template, evaluator)) {
          if (evaluator.callbackMode) evaluator.popInstanceToken();
          if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
          return false;
        }
      }
    }
    if (evaluator.callbackMode) evaluator.popInstanceToken();
  }
  evaluator.propertyParent = undefined;
  evaluator.propertyKey = undefined;
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function LoopPropertiesExcept(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const filter = instruction[5];
  const filterStrings = filter[0];
  const filterPrefixes = filter[1];
  const filterRegexes = filter[2];
  const children = instruction[6];
  for (const key in target) {
    if (filterStrings.has(key)) continue;
    let matched = false;
    for (let index = 0; index < filterPrefixes.length; index++) {
      if (key.startsWith(filterPrefixes[index])) { matched = true; break; }
    }
    if (matched) continue;
    for (let index = 0; index < filterRegexes.length; index++) {
      filterRegexes[index].lastIndex = 0;
      if (filterRegexes[index].test(key)) { matched = true; break; }
    }
    if (matched) continue;
    evaluator.propertyParent = target;
    evaluator.propertyKey = key;
    if (evaluator.callbackMode) evaluator.pushInstanceToken(key);
    if (children) {
      for (let childIndex = 0; childIndex < children.length; childIndex++) {
        if (!evaluateInstruction(children[childIndex], target[key], depth + 1, template, evaluator)) {
          if (evaluator.callbackMode) evaluator.popInstanceToken();
          if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
          return false;
        }
      }
    }
    if (evaluator.callbackMode) evaluator.popInstanceToken();
  }
  evaluator.propertyParent = undefined;
  evaluator.propertyKey = undefined;
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function LoopPropertiesType(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const expected = instruction[5];
  for (const key in target) {
    const actual = jsonTypeOf(target[key]);
    if (actual !== expected && !(expected === Type.Integer && isIntegral(target[key]))) {
      if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
      return false;
    }
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function LoopPropertiesTypeEvaluate(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const expected = instruction[5];
  for (const key in target) {
    const actual = jsonTypeOf(target[key]);
    if (actual !== expected && !(expected === Type.Integer && isIntegral(target[key]))) {
      if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
      return false;
    }
  }
  if (evaluator.trackMode) evaluator.markEvaluated(target);
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function LoopPropertiesExactlyTypeStrict(instruction, instance, depth, template, evaluator) {
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) {
    if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
    return false;
  }
  const value = instruction[5];
  let count = 0;
  for (const key in target) {
    count++;
    if (effectiveTypeStrictReal(target[key]) !== value[0]) {
      if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
      return false;
    }
  }
  const __result = count === value[1].length;
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, __result);
  return __result;
};

function LoopPropertiesExactlyTypeStrictHash(instruction, instance, depth, template, evaluator) {
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) {
    if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
    return false;
  }
  const value = instruction[5];
  const entries = value[1][0];
  const expectedCount = entries.length;
  let count = 0;
  for (const key in target) {
    count++;
    if (effectiveTypeStrictReal(target[key]) !== value[0]) {
      if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
      return false;
    }
  }
  if (count !== expectedCount) {
    if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
    return false;
  }
  for (let index = 0; index < expectedCount; index++) {
    if (!Object.hasOwn(target, entries[index][1])) {
      if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
      return false;
    }
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function LoopPropertiesTypeStrict(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const expected = instruction[5];
  for (const key in target) {
    if (effectiveTypeStrictReal(target[key]) !== expected) {
      if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
      return false;
    }
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function LoopPropertiesTypeStrictEvaluate(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const expected = instruction[5];
  for (const key in target) {
    if (effectiveTypeStrictReal(target[key]) !== expected) {
      if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
      return false;
    }
  }
  if (evaluator.trackMode) evaluator.markEvaluated(target);
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function LoopPropertiesTypeStrictAny(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const bitmask = instruction[5];
  for (const key in target) {
    if (!typeSetTest(bitmask, effectiveTypeStrictReal(target[key]))) {
      if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
      return false;
    }
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function LoopPropertiesTypeStrictAnyEvaluate(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const bitmask = instruction[5];
  for (const key in target) {
    if (!typeSetTest(bitmask, effectiveTypeStrictReal(target[key]))) {
      if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
      return false;
    }
  }
  if (evaluator.trackMode) evaluator.markEvaluated(target);
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function LoopKeys(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const children = instruction[6];
  for (const key in target) {
    if (evaluator.callbackMode) evaluator.pushInstanceToken(key);
    const previousPropertyTarget = evaluator.propertyTarget;
    evaluator.propertyTarget = key;
    if (children) {
      for (let childIndex = 0; childIndex < children.length; childIndex++) {
        if (!evaluateInstruction(children[childIndex], null, depth + 1, template, evaluator)) {
          evaluator.propertyTarget = previousPropertyTarget;
          if (evaluator.callbackMode) evaluator.popInstanceToken();
          if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
          return false;
        }
      }
    }
    evaluator.propertyTarget = previousPropertyTarget;
    if (evaluator.callbackMode) evaluator.popInstanceToken();
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function LoopItems(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!Array.isArray(target)) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const children = instruction[6];
  for (let index = 0; index < target.length; index++) {
    if (evaluator.callbackMode) evaluator.pushInstanceToken(index);
    if (children) {
      for (let childIndex = 0; childIndex < children.length; childIndex++) {
        if (!evaluateInstruction(children[childIndex], target[index], depth + 1, template, evaluator)) {
          if (evaluator.callbackMode) evaluator.popInstanceToken();
          if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
          return false;
        }
      }
    }
    if (evaluator.callbackMode) evaluator.popInstanceToken();
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function LoopItemsFrom(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  const startIndex = instruction[5];
  if (!Array.isArray(target) || startIndex >= target.length) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const children = instruction[6];
  for (let index = startIndex; index < target.length; index++) {
    if (evaluator.callbackMode) evaluator.pushInstanceToken(index);
    if (children) {
      for (let childIndex = 0; childIndex < children.length; childIndex++) {
        if (!evaluateInstruction(children[childIndex], target[index], depth + 1, template, evaluator)) {
          if (evaluator.callbackMode) evaluator.popInstanceToken();
          if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
          return false;
        }
      }
    }
    if (evaluator.callbackMode) evaluator.popInstanceToken();
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function LoopItemsUnevaluated(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!Array.isArray(target)) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  if (evaluator.trackMode && evaluator.isEvaluated(target)) {
    if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
    return true;
  }
  const children = instruction[6];
  for (let index = 0; index < target.length; index++) {
    if (evaluator.trackMode && evaluator.isEvaluated(target[index], target, index)) continue;
    if (evaluator.callbackMode) evaluator.pushInstanceToken(index);
    if (children) {
      for (let childIndex = 0; childIndex < children.length; childIndex++) {
        if (!evaluateInstruction(children[childIndex], target[index], depth + 1, template, evaluator)) {
          if (evaluator.callbackMode) evaluator.popInstanceToken();
          if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
          return false;
        }
      }
    }
    if (evaluator.callbackMode) evaluator.popInstanceToken();
  }
  if (evaluator.trackMode) evaluator.markEvaluated(target);
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function LoopItemsType(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!Array.isArray(target)) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const expected = instruction[5];
  for (let index = 0; index < target.length; index++) {
    const actual = jsonTypeOf(target[index]);
    if (actual !== expected && !(expected === Type.Integer && isIntegral(target[index]))) {
      if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
      return false;
    }
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function LoopItemsTypeStrict(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!Array.isArray(target)) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const expected = instruction[5];
  for (let index = 0; index < target.length; index++) {
    if (effectiveTypeStrictReal(target[index]) !== expected) {
      if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
      return false;
    }
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function LoopItemsTypeStrictAny(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!Array.isArray(target)) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const bitmask = instruction[5];
  for (let index = 0; index < target.length; index++) {
    if (!typeSetTest(bitmask, effectiveTypeStrictReal(target[index]))) {
      if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
      return false;
    }
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function LoopItemsPropertiesExactlyTypeStrictHash(instruction, instance, depth, template, evaluator) {
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const target = resolveInstance(instance, instruction[2]);
  if (!Array.isArray(target)) {
    if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
    return false;
  }
  const expectedType = instruction[5][0];
  const entries = instruction[5][1][0];
  const expectedCount = entries.length;
  for (let index = 0; index < target.length; index++) {
    const item = target[index];
    if (!isObject(item)) {
      if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
      return false;
    }
    let count = 0;
    for (const key in item) {
      count++;
      if (effectiveTypeStrictReal(item[key]) !== expectedType) {
        if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
        return false;
      }
    }
    if (count !== expectedCount) {
      if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
      return false;
    }
    for (let entry = 0; entry < expectedCount; entry++) {
      if (!Object.hasOwn(item, entries[entry][1])) {
        if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
        return false;
      }
    }
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function LoopItemsIntegerBounded(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!Array.isArray(target) || target.length === 0) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const minimum = instruction[5][0];
  const maximum = instruction[5][1];
  for (let index = 0; index < target.length; index++) {
    const element = target[index];
    const elementType = typeof element;
    if (elementType !== 'number' && elementType !== 'bigint') {
      if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
      return false;
    }
    if (element < minimum || element > maximum) {
      if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
      return false;
    }
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function LoopItemsIntegerBoundedSized(instruction, instance, depth, template, evaluator) {
  const value = instruction[5];
  const minimum = value[0][0];
  const maximum = value[0][1];
  const minimumSize = value[1][0];
  const target = resolveInstance(instance, instruction[2]);
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  if (!Array.isArray(target) || target.length < minimumSize) {
    if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
    return false;
  }
  for (let index = 0; index < target.length; index++) {
    const element = target[index];
    const elementType = typeof element;
    if (elementType !== 'number' && elementType !== 'bigint') {
      if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
      return false;
    }
    if (element < minimum || element > maximum) {
      if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
      return false;
    }
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

function LoopContains(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!Array.isArray(target)) return true;
  const range = instruction[5];
  const minimum = range[0];
  const maximum = range[1];
  const isExhaustive = range[2];
  if (minimum === 0 && target.length === 0) return true;
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);

  const children = instruction[6];
  let result = false;
  let matchCount = 0;
  for (let index = 0; index < target.length; index++) {
    if (evaluator.callbackMode) evaluator.pushInstanceToken(index);
    let subresult = true;
    if (children) {
      for (let childIndex = 0; childIndex < children.length; childIndex++) {
        if (!evaluateInstruction(children[childIndex], target[index], depth + 1, template, evaluator)) {
          subresult = false;
          break;
        }
      }
    }
    if (evaluator.callbackMode) evaluator.popInstanceToken();
    if (subresult) {
      matchCount++;
      if (maximum !== null && matchCount > maximum) {
        result = false;
        break;
      }
      if (matchCount >= minimum) {
        result = true;
        if (maximum === null && !isExhaustive) break;
      }
    }
  }

  if (evaluator.callbackMode) evaluator.callbackPop(instruction, result);
  return result;
};

function ControlGroup(instruction, instance, depth, template, evaluator) {
  const children = instruction[6];
  if (children) {
    for (let index = 0; index < children.length; index++) {
      if (!evaluateInstruction(children[index], instance, depth + 1, template, evaluator)) return false;
    }
  }
  return true;
};

function ControlGroupWhenDefines(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return true;
  if (!Object.hasOwn(target, instruction[5])) return true;
  const children = instruction[6];
  if (children) {
    for (let index = 0; index < children.length; index++) {
      if (!evaluateInstruction(children[index], instance, depth + 1, template, evaluator)) return false;
    }
  }
  return true;
};

function ControlGroupWhenDefinesDirect(instruction, instance, depth, template, evaluator) {
  if (!isObject(instance)) return true;
  if (!Object.hasOwn(instance, instruction[5])) return true;
  const children = instruction[6];
  if (children) {
    for (let index = 0; index < children.length; index++) {
      if (!evaluateInstruction(children[index], instance, depth + 1, template, evaluator)) return false;
    }
  }
  return true;
};

function ControlGroupWhenType(instruction, instance, depth, template, evaluator) {
  if (jsonTypeOf(instance) !== instruction[5]) return true;
  const children = instruction[6];
  if (children) {
    for (let index = 0; index < children.length; index++) {
      if (!evaluateInstruction(children[index], instance, depth + 1, template, evaluator)) return false;
    }
  }
  return true;
};

function ControlEvaluate(instruction, instance, depth, template, evaluator) {
  if (evaluator.trackMode) {
    const target = resolveInstance(instance, instruction[5]);
    evaluator.markEvaluated(target, evaluator.propertyParent, evaluator.propertyKey);
  }
  return true;
};

function ControlDynamicAnchorJump(instruction, instance, depth, template, evaluator) {
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const resolved = resolveInstance(instance, instruction[2]);
  const anchor = instruction[5];

  if (!evaluator.resources) {
    if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
    return false;
  }

  const anchors = template[5];
  for (let index = 0; index < evaluator.resources.length; index++) {
    const jumpTarget = anchors.get(evaluator.resources[index] + ':' + anchor);
    if (jumpTarget !== undefined) {
      for (let childIndex = 0; childIndex < jumpTarget.length; childIndex++) {
        if (!evaluateInstruction(jumpTarget[childIndex], resolved, depth + 1, template, evaluator)) {
          if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
          return false;
        }
      }
      if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
      return true;
    }
  }

  if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
  return false;
};

function ControlJump(instruction, instance, depth, template, evaluator) {
  if (evaluator.callbackMode) evaluator.callbackPush(instruction);
  const jumpTarget = instruction[5];
  if (!jumpTarget) {
    if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
    return true;
  }
  const resolved = resolveInstance(instance, instruction[2]);
  for (let index = 0; index < jumpTarget.length; index++) {
    if (!evaluateInstruction(jumpTarget[index], resolved, depth + 1, template, evaluator)) {
      if (evaluator.callbackMode) evaluator.callbackPop(instruction, false);
      return false;
    }
  }
  if (evaluator.callbackMode) evaluator.callbackPop(instruction, true);
  return true;
};

const handlers = [
  AssertionFail,                              // 0
  AssertionDefines,                           // 1
  AssertionDefinesStrict,                     // 2
  AssertionDefinesAll,                        // 3
  AssertionDefinesAllStrict,                  // 4
  AssertionDefinesExactly,                    // 5
  AssertionDefinesExactlyStrict,              // 6
  AssertionDefinesExactlyStrictHash3,         // 7
  AssertionPropertyDependencies,              // 8
  AssertionType,                              // 9
  AssertionTypeAny,                           // 10
  AssertionTypeStrict,                        // 11
  AssertionTypeStrictAny,                     // 12
  AssertionTypeStringBounded,                 // 13
  AssertionTypeStringUpper,                   // 14
  AssertionTypeArrayBounded,                  // 15
  AssertionTypeArrayUpper,                    // 16
  AssertionTypeObjectBounded,                 // 17
  AssertionTypeObjectUpper,                   // 18
  AssertionRegex,                             // 19
  AssertionStringSizeLess,                    // 20
  AssertionStringSizeGreater,                 // 21
  AssertionArraySizeLess,                     // 22
  AssertionArraySizeGreater,                  // 23
  AssertionObjectSizeLess,                    // 24
  AssertionObjectSizeGreater,                 // 25
  AssertionEqual,                             // 26
  AssertionEqualsAny,                         // 27
  AssertionEqualsAnyStringHash,               // 28
  AssertionGreaterEqual,                      // 29
  AssertionLessEqual,                         // 30
  AssertionGreater,                           // 31
  AssertionLess,                              // 32
  AssertionUnique,                            // 33
  AssertionDivisible,                         // 34
  AssertionTypeIntegerBounded,                // 35
  AssertionTypeIntegerBoundedStrict,          // 36
  AssertionTypeIntegerLowerBound,             // 37
  AssertionTypeIntegerLowerBoundStrict,       // 38
  AssertionStringType,                        // 39
  AssertionPropertyType,                      // 40
  AssertionPropertyTypeEvaluate,              // 41
  AssertionPropertyTypeStrict,                // 42
  AssertionPropertyTypeStrictEvaluate,        // 43
  AssertionPropertyTypeStrictAny,             // 44
  AssertionPropertyTypeStrictAnyEvaluate,     // 45
  AssertionArrayPrefix,                       // 46
  AssertionArrayPrefixEvaluate,               // 47
  AssertionObjectPropertiesSimple,            // 48
  AnnotationEmit,                             // 49
  AnnotationToParent,                         // 50
  AnnotationBasenameToParent,                 // 51
  Evaluate,                                   // 52
  LogicalNot,                                 // 53
  LogicalNotEvaluate,                         // 54
  LogicalOr,                                  // 55
  LogicalAnd,                                 // 56
  LogicalXor,                                 // 57
  LogicalCondition,                           // 58
  LogicalWhenType,                            // 59
  LogicalWhenDefines,                         // 60
  LogicalWhenArraySizeGreater,                // 61
  LoopPropertiesUnevaluated,                  // 62
  LoopPropertiesUnevaluatedExcept,            // 63
  LoopPropertiesMatch,                        // 64
  LoopPropertiesMatchClosed,                  // 65
  LoopProperties,                             // 66
  LoopPropertiesEvaluate,                     // 67
  LoopPropertiesRegex,                        // 68
  LoopPropertiesRegexClosed,                  // 69
  LoopPropertiesStartsWith,                   // 70
  LoopPropertiesExcept,                       // 71
  LoopPropertiesType,                         // 72
  LoopPropertiesTypeEvaluate,                 // 73
  LoopPropertiesExactlyTypeStrict,            // 74
  LoopPropertiesExactlyTypeStrictHash,        // 75
  LoopPropertiesTypeStrict,                   // 76
  LoopPropertiesTypeStrictEvaluate,           // 77
  LoopPropertiesTypeStrictAny,                // 78
  LoopPropertiesTypeStrictAnyEvaluate,        // 79
  LoopKeys,                                   // 80
  LoopItems,                                  // 81
  LoopItemsFrom,                              // 82
  LoopItemsUnevaluated,                       // 83
  LoopItemsType,                              // 84
  LoopItemsTypeStrict,                        // 85
  LoopItemsTypeStrictAny,                     // 86
  LoopItemsPropertiesExactlyTypeStrictHash,   // 87
  LoopItemsPropertiesExactlyTypeStrictHash,   // 88
  LoopItemsIntegerBounded,                    // 89
  LoopItemsIntegerBoundedSized,               // 90
  LoopContains,                               // 91
  ControlGroup,                               // 92
  ControlGroupWhenDefines,                    // 93
  ControlGroupWhenDefinesDirect,              // 94
  ControlGroupWhenType,                       // 95
  ControlEvaluate,                            // 96
  ControlDynamicAnchorJump,                   // 97
  ControlJump                                 // 98
];

function AssertionTypeArrayBounded_fast(instruction, instance, depth, template, evaluator) {
  const relInstance = instruction[2];
  const target = relInstance.length === 0 ? instance : resolveInstance(instance, relInstance);
  if (!Array.isArray(target)) return false;
  const range = instruction[5];
  if (target.length < range[0]) return false;
  if (range[1] !== null && target.length > range[1]) return false;
  return true;
}

function LoopItemsTypeStrictAny_fast(instruction, instance, depth, template, evaluator) {
  const relInstance = instruction[2];
  const target = relInstance.length === 0 ? instance : resolveInstance(instance, relInstance);
  if (!Array.isArray(target)) return true;
  const bitmask = instruction[5];
  for (let index = 0; index < target.length; index++) {
    if (!typeSetTest(bitmask, effectiveTypeStrictReal(target[index]))) return false;
  }
  return true;
}

function AssertionPropertyTypeStrict_fast(instruction, instance, depth, template, evaluator) {
  if (!isObject(instance)) return true;
  const target = resolveInstance(instance, instruction[2]);
  if (target === undefined) return true;
  return effectiveTypeStrictReal(target) === instruction[5];
}

function AssertionTypeStrict_fast(instruction, instance, depth, template, evaluator) {
  const relInstance = instruction[2];
  const target = relInstance.length === 0 ? instance : resolveInstance(instance, relInstance);
  return effectiveTypeStrictReal(target) === instruction[5];
}

function AssertionDefinesAllStrict_fast(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return false;
  const strings = instruction[5];
  for (let index = 0; index < strings.length; index++) {
    if (!Object.hasOwn(target, strings[index])) return false;
  }
  return true;
}

function AssertionEqual_fast(instruction, instance, depth, template, evaluator) {
  if (evaluator.propertyTarget !== undefined) {
    const value = instruction[5];
    return typeof value === 'string' && value === evaluator.propertyTarget;
  }
  return jsonEqual(resolveInstance(instance, instruction[2]), instruction[5]);
}

function LoopPropertiesMatch_fast(instruction, instance, depth, template, evaluator) {
  const relInstance = instruction[2];
  const target = relInstance.length === 0 ? instance : resolveInstance(instance, relInstance);
  if (!isObject(target)) return true;
  const children = instruction[6];
  for (const key in target) {
    const index = instruction[5][key];
    if (index === undefined) continue;
    const subinstruction = children[index];
    const subchildren = subinstruction[6];
    if (subchildren) {
      for (let childIndex = 0; childIndex < subchildren.length; childIndex++) {
        if (!evaluateInstruction(subchildren[childIndex], target, depth + 1, template, evaluator)) return false;
      }
    }
  }
  return true;
}

function LogicalOr_fast(instruction, instance, depth, template, evaluator) {
  const children = instruction[6];
  if (!children || children.length === 0) return true;
  const relInstance = instruction[2];
  const target = relInstance.length === 0 ? instance : resolveInstance(instance, relInstance);
  const exhaustive = instruction[5];
  let result = false;
  if (exhaustive) {
    for (let index = 0; index < children.length; index++) {
      if (evaluateInstruction(children[index], target, depth + 1, template, evaluator)) result = true;
    }
  } else {
    for (let index = 0; index < children.length; index++) {
      if (evaluateInstruction(children[index], target, depth + 1, template, evaluator)) return true;
    }
  }
  return result;
}

function ControlJump_fast(instruction, instance, depth, template, evaluator) {
  const jumpTarget = instruction[5];
  if (!jumpTarget) return true;
  const relInstance = instruction[2];
  const resolved = relInstance.length === 0 ? instance : resolveInstance(instance, relInstance);
  for (let index = 0; index < jumpTarget.length; index++) {
    if (!evaluateInstruction(jumpTarget[index], resolved, depth + 1, template, evaluator)) return false;
  }
  return true;
}

function AssertionEqualsAnyStringHash_fast(instruction, instance, depth, template, evaluator) {
  const target = evaluator.propertyTarget !== undefined
    ? evaluator.propertyTarget
    : resolveInstance(instance, instruction[2]);
  if (typeof target !== 'string') return false;
  const value = instruction[5];
  const entries = value[0];
  const tableOfContents = value[1];
  const stringSize = target.length;
  if (stringSize < tableOfContents.length) {
    const hint = tableOfContents[stringSize];
    if (hint[1] === 0) return false;
    for (let index = hint[0] - 1; index < hint[1]; index++) {
      if (entries[index][1] === target) return true;
    }
  }
  return false;
}

function LogicalXor_fast(instruction, instance, depth, template, evaluator) {
  const relInstance = instruction[2];
  const target = relInstance.length === 0 ? instance : resolveInstance(instance, relInstance);
  const exhaustive = instruction[5];
  const children = instruction[6];
  let result = true;
  let hasMatched = false;
  if (children) {
    for (let index = 0; index < children.length; index++) {
      if (evaluateInstruction(children[index], target, depth + 1, template, evaluator)) {
        if (hasMatched) {
          result = false;
          if (!exhaustive) break;
        } else {
          hasMatched = true;
        }
      }
    }
  }
  return result && hasMatched;
}

function AssertionDefinesStrict_fast(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  return isObject(target) && Object.hasOwn(target, instruction[5]);
}

function LoopItems_fast(instruction, instance, depth, template, evaluator) {
  const relInstance = instruction[2];
  const target = relInstance.length === 0 ? instance : resolveInstance(instance, relInstance);
  if (!Array.isArray(target)) return true;
  const children = instruction[6];
  for (let index = 0; index < target.length; index++) {
    if (children) {
      for (let childIndex = 0; childIndex < children.length; childIndex++) {
        if (!evaluateInstruction(children[childIndex], target[index], depth + 1, template, evaluator)) return false;
      }
    }
  }
  return true;
}

function LoopPropertiesMatchClosed_fast(instruction, instance, depth, template, evaluator) {
  const relInstance = instruction[2];
  const target = relInstance.length === 0 ? instance : resolveInstance(instance, relInstance);
  if (!isObject(target)) return true;
  const children = instruction[6];
  for (const key in target) {
    const index = instruction[5][key];
    if (index === undefined) return false;
    const subinstruction = children[index];
    const subchildren = subinstruction[6];
    if (subchildren) {
      for (let childIndex = 0; childIndex < subchildren.length; childIndex++) {
        if (!evaluateInstruction(subchildren[childIndex], target, depth + 1, template, evaluator)) return false;
      }
    }
  }
  return true;
}

function LogicalAnd_fast(instruction, instance, depth, template, evaluator) {
  const relInstance = instruction[2];
  const target = relInstance.length === 0 ? instance : resolveInstance(instance, relInstance);
  const children = instruction[6];
  if (children) {
    for (let index = 0; index < children.length; index++) {
      if (!evaluateInstruction(children[index], target, depth + 1, template, evaluator)) return false;
    }
  }
  return true;
}

function AssertionTypeStringBounded_fast(instruction, instance, depth, template, evaluator) {
  const relInstance = instruction[2];
  const target = relInstance.length === 0 ? instance : resolveInstance(instance, relInstance);
  if (typeof target !== 'string') return false;
  const range = instruction[5];
  const length = unicodeLength(target);
  if (length < range[0]) return false;
  return range[1] === null || length <= range[1];
}

function AssertionPropertyDependencies_fast(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return true;
  const value = instruction[5];
  for (const property in value) {
    if (!Object.hasOwn(target, property)) continue;
    const dependencies = value[property];
    for (let index = 0; index < dependencies.length; index++) {
      if (!Object.hasOwn(target, dependencies[index])) return false;
    }
  }
  return true;
}

function AssertionTypeAny_fast(instruction, instance, depth, template, evaluator) {
  const relInstance = instruction[2];
  const target = relInstance.length === 0 ? instance : resolveInstance(instance, relInstance);
  const bitmask = instruction[5];
  const typeIndex = jsonTypeOf(target);
  if (typeSetTest(bitmask, typeIndex)) return true;
  return typeSetTest(bitmask, Type.Integer) && isIntegral(target);
}

function LogicalCondition_fast(instruction, instance, depth, template, evaluator) {
  const value = instruction[5];
  const thenStart = value[0];
  const elseStart = value[1];
  const children = instruction[6];
  const childrenSize = children ? children.length : 0;
  let conditionEnd = childrenSize;
  if (thenStart > 0) conditionEnd = thenStart;
  else if (elseStart > 0) conditionEnd = elseStart;
  const relInstance = instruction[2];
  const target = relInstance.length === 0 ? instance : resolveInstance(instance, relInstance);
  let conditionResult = true;
  for (let cursor = 0; cursor < conditionEnd; cursor++) {
    if (!evaluateInstruction(children[cursor], target, depth + 1, template, evaluator)) {
      conditionResult = false;
      break;
    }
  }
  const consequenceStart = conditionResult ? thenStart : elseStart;
  const consequenceEnd = (conditionResult && elseStart > 0) ? elseStart : childrenSize;
  if (consequenceStart > 0) {
    if (evaluator.trackMode) {
      evaluator.popPath(instruction[1].length);
    }
    let result = true;
    for (let cursor = consequenceStart; cursor < consequenceEnd; cursor++) {
      if (!evaluateInstruction(children[cursor], target, depth + 1, template, evaluator)) {
        result = false;
        break;
      }
    }
    if (evaluator.trackMode) {
      evaluator.pushPath(instruction[1]);
    }
    return result;
  }
  return true;
}

function LoopPropertiesExcept_fast(instruction, instance, depth, template, evaluator) {
  const relInstance = instruction[2];
  const target = relInstance.length === 0 ? instance : resolveInstance(instance, relInstance);
  if (!isObject(target)) return true;
  const filter = instruction[5];
  const filterStrings = filter[0];
  const filterPrefixes = filter[1];
  const filterRegexes = filter[2];
  const children = instruction[6];
  for (const key in target) {
    if (filterStrings.has(key)) continue;
    let matched = false;
    for (let index = 0; index < filterPrefixes.length; index++) {
      if (key.startsWith(filterPrefixes[index])) { matched = true; break; }
    }
    if (matched) continue;
    for (let index = 0; index < filterRegexes.length; index++) {
      filterRegexes[index].lastIndex = 0;
      if (filterRegexes[index].test(key)) { matched = true; break; }
    }
    if (matched) continue;
    evaluator.propertyParent = target;
    evaluator.propertyKey = key;
    if (children) {
      for (let childIndex = 0; childIndex < children.length; childIndex++) {
        if (!evaluateInstruction(children[childIndex], target[key], depth + 1, template, evaluator)) {
          evaluator.propertyParent = undefined;
          evaluator.propertyKey = undefined;
          return false;
        }
      }
    }
  }
  evaluator.propertyParent = undefined;
  evaluator.propertyKey = undefined;
  return true;
}

function AssertionRegex_fast(instruction, instance, depth, template, evaluator) {
  const target = evaluator.propertyTarget !== undefined
    ? evaluator.propertyTarget : resolveInstance(instance, instruction[2]);
  if (typeof target !== 'string') return true;
  return instruction[5].test(target);
}

function LoopProperties_fast(instruction, instance, depth, template, evaluator) {
  const relInstance = instruction[2];
  const target = relInstance.length === 0 ? instance : resolveInstance(instance, relInstance);
  if (!isObject(target)) return true;
  const children = instruction[6];
  for (const key in target) {
    evaluator.propertyParent = target;
    evaluator.propertyKey = key;
    if (children) {
      for (let childIndex = 0; childIndex < children.length; childIndex++) {
        if (!evaluateInstruction(children[childIndex], target[key], depth + 1, template, evaluator)) {
          evaluator.propertyParent = undefined;
          evaluator.propertyKey = undefined;
          return false;
        }
      }
    }
  }
  evaluator.propertyParent = undefined;
  evaluator.propertyKey = undefined;
  return true;
}

function AssertionDefines_fast(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return true;
  return Object.hasOwn(target, instruction[5]);
}

function LogicalWhenType_fast(instruction, instance, depth, template, evaluator) {
  const relInstance = instruction[2];
  const target = relInstance.length === 0 ? instance : resolveInstance(instance, relInstance);
  if (jsonTypeOf(target) !== instruction[5]) return true;
  const children = instruction[6];
  if (children) {
    for (let index = 0; index < children.length; index++) {
      if (!evaluateInstruction(children[index], target, depth + 1, template, evaluator)) return false;
    }
  }
  return true;
}

function LogicalWhenDefines_fast(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return true;
  if (!Object.hasOwn(target, instruction[5])) return true;
  const children = instruction[6];
  if (children) {
    for (let index = 0; index < children.length; index++) {
      if (!evaluateInstruction(children[index], target, depth + 1, template, evaluator)) return false;
    }
  }
  return true;
}

function AssertionFail_fast() { return false; }

function LoopContains_fast(instruction, instance, depth, template, evaluator) {
  const relInstance = instruction[2];
  const target = relInstance.length === 0 ? instance : resolveInstance(instance, relInstance);
  if (!Array.isArray(target)) return true;
  const range = instruction[5];
  const minimum = range[0];
  const maximum = range[1];
  const isExhaustive = range[2];
  if (minimum === 0 && target.length === 0) return true;
  const children = instruction[6];
  let matchCount = 0;
  for (let index = 0; index < target.length; index++) {
    let subresult = true;
    if (children) {
      for (let childIndex = 0; childIndex < children.length; childIndex++) {
        if (!evaluateInstruction(children[childIndex], target[index], depth + 1, template, evaluator)) {
          subresult = false;
          break;
        }
      }
    }
    if (subresult) {
      matchCount++;
      if (maximum !== null && matchCount > maximum) return false;
      if (matchCount >= minimum && maximum === null && !isExhaustive) return true;
    }
  }
  return matchCount >= minimum;
}

function LogicalNot_fast(instruction, instance, depth, template, evaluator) {
  const relInstance = instruction[2];
  const target = relInstance.length === 0 ? instance : resolveInstance(instance, relInstance);
  const children = instruction[6];
  if (children) {
    for (let index = 0; index < children.length; index++) {
      if (!evaluateInstruction(children[index], target, depth + 1, template, evaluator)) return true;
    }
  }
  return false;
}

function LoopItemsType_fast(instruction, instance, depth, template, evaluator) {
  const relInstance = instruction[2];
  const target = relInstance.length === 0 ? instance : resolveInstance(instance, relInstance);
  if (!Array.isArray(target)) return true;
  const expected = instruction[5];
  for (let index = 0; index < target.length; index++) {
    const actual = jsonTypeOf(target[index]);
    if (actual !== expected && !(expected === Type.Integer && isIntegral(target[index]))) return false;
  }
  return true;
}

function LoopItemsTypeStrict_fast(instruction, instance, depth, template, evaluator) {
  const relInstance = instruction[2];
  const target = relInstance.length === 0 ? instance : resolveInstance(instance, relInstance);
  if (!Array.isArray(target)) return true;
  const expected = instruction[5];
  for (let index = 0; index < target.length; index++) {
    if (effectiveTypeStrictReal(target[index]) !== expected) return false;
  }
  return true;
}

function AssertionEqualsAny_fast(instruction, instance, depth, template, evaluator) {
  const value = instruction[5];
  const target = evaluator.propertyTarget !== undefined
    ? evaluator.propertyTarget : resolveInstance(instance, instruction[2]);
  if (value.primitive) return value.set.has(target);
  const values = Array.isArray(value) ? value : value.values;
  for (let index = 0; index < values.length; index++) {
    if (jsonEqual(target, values[index])) return true;
  }
  return false;
}

function AssertionDefinesAll_fast(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return true;
  const strings = instruction[5];
  for (let index = 0; index < strings.length; index++) {
    if (!Object.hasOwn(target, strings[index])) return false;
  }
  return true;
}

function AssertionDefinesExactly_fast(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return true;
  let targetSize = 0;
  for (const key in target) targetSize++;
  const strings = instruction[5];
  if (targetSize !== strings.length) return false;
  for (let index = 0; index < strings.length; index++) {
    if (!Object.hasOwn(target, strings[index])) return false;
  }
  return true;
}

function AssertionDefinesExactlyStrict_fast(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return false;
  let targetSize = 0;
  for (const key in target) targetSize++;
  const strings = instruction[5];
  if (targetSize !== strings.length) return false;
  for (let index = 0; index < strings.length; index++) {
    if (!Object.hasOwn(target, strings[index])) return false;
  }
  return true;
}

function AssertionDefinesExactlyStrictHash3_fast(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return false;
  const entries = instruction[5][0];
  let count = 0;
  for (const key in target) count++;
  if (count !== 3) return false;
  return Object.hasOwn(target, entries[0][1]) &&
    Object.hasOwn(target, entries[1][1]) &&
    Object.hasOwn(target, entries[2][1]);
}

function AssertionType_fast(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  const expected = instruction[5];
  const actual = jsonTypeOf(target);
  if (actual === expected) return true;
  return expected === Type.Integer && isIntegral(target);
}

function AssertionTypeStrictAny_fast(instruction, instance, depth, template, evaluator) {
  const relInstance = instruction[2];
  const target = relInstance.length === 0 ? instance : resolveInstance(instance, relInstance);
  return typeSetTest(instruction[5], effectiveTypeStrictReal(target));
}

function AssertionTypeStringUpper_fast(instruction, instance, depth, template, evaluator) {
  const relInstance = instruction[2];
  const target = relInstance.length === 0 ? instance : resolveInstance(instance, relInstance);
  return typeof target === 'string' && unicodeLength(target) <= instruction[5];
}

function AssertionTypeArrayUpper_fast(instruction, instance, depth, template, evaluator) {
  const relInstance = instruction[2];
  const target = relInstance.length === 0 ? instance : resolveInstance(instance, relInstance);
  return Array.isArray(target) && target.length <= instruction[5];
}

function AssertionTypeObjectBounded_fast(instruction, instance, depth, template, evaluator) {
  const relInstance = instruction[2];
  const target = relInstance.length === 0 ? instance : resolveInstance(instance, relInstance);
  if (!isObject(target)) return false;
  const range = instruction[5];
  const size = objectSize(target);
  if (size < range[0]) return false;
  return range[1] === null || size <= range[1];
}

function AssertionTypeObjectUpper_fast(instruction, instance, depth, template, evaluator) {
  const relInstance = instruction[2];
  const target = relInstance.length === 0 ? instance : resolveInstance(instance, relInstance);
  if (!isObject(target)) return false;
  return objectSize(target) <= instruction[5];
}

function AssertionStringSizeLess_fast(instruction, instance, depth, template, evaluator) {
  const target = evaluator.propertyTarget !== undefined
    ? evaluator.propertyTarget : resolveInstance(instance, instruction[2]);
  if (typeof target !== 'string') return true;
  return unicodeLength(target) < instruction[5];
}

function AssertionStringSizeGreater_fast(instruction, instance, depth, template, evaluator) {
  const target = evaluator.propertyTarget !== undefined
    ? evaluator.propertyTarget : resolveInstance(instance, instruction[2]);
  if (typeof target !== 'string') return true;
  return unicodeLength(target) > instruction[5];
}

function AssertionArraySizeLess_fast(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!Array.isArray(target)) return true;
  return target.length < instruction[5];
}

function AssertionArraySizeGreater_fast(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!Array.isArray(target)) return true;
  return target.length > instruction[5];
}

function AssertionObjectSizeLess_fast(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return true;
  return objectSize(target) < instruction[5];
}

function AssertionObjectSizeGreater_fast(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return true;
  return objectSize(target) > instruction[5];
}

function AssertionGreaterEqual_fast(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  const targetType = typeof target;
  if (targetType !== 'number' && targetType !== 'bigint') return true;
  return target >= instruction[5];
}

function AssertionLessEqual_fast(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  const targetType = typeof target;
  if (targetType !== 'number' && targetType !== 'bigint') return true;
  return target <= instruction[5];
}

function AssertionGreater_fast(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  const targetType = typeof target;
  if (targetType !== 'number' && targetType !== 'bigint') return true;
  return target > instruction[5];
}

function AssertionLess_fast(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  const targetType = typeof target;
  if (targetType !== 'number' && targetType !== 'bigint') return true;
  return target < instruction[5];
}

function AssertionUnique_fast(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!Array.isArray(target)) return true;
  return isUnique(target);
}

function AssertionDivisible_fast(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  const targetType = typeof target;
  if (targetType !== 'number' && targetType !== 'bigint') return true;
  return isDivisibleBy(target, instruction[5]);
}

function AssertionStringType_fast(instruction, instance, depth, template, evaluator) {
  const target = evaluator.propertyTarget !== undefined
    ? evaluator.propertyTarget : resolveInstance(instance, instruction[2]);
  if (typeof target !== 'string') return true;
  return URI_REGEX.test(target);
}

function AssertionPropertyType_fast(instruction, instance, depth, template, evaluator) {
  if (!isObject(instance)) return true;
  const target = resolveInstance(instance, instruction[2]);
  if (target === undefined) return true;
  const expected = instruction[5];
  const actual = jsonTypeOf(target);
  return actual === expected || (expected === Type.Integer && isIntegral(target));
}

function AssertionPropertyTypeEvaluate_fast(instruction, instance, depth, template, evaluator) {
  if (!isObject(instance)) return true;
  const target = resolveInstance(instance, instruction[2]);
  if (target === undefined) return true;
  const expected = instruction[5];
  const actual = jsonTypeOf(target);
  const result = actual === expected || (expected === Type.Integer && isIntegral(target));
  if (result && evaluator.trackMode) {
    const location = instruction[2];
    evaluator.markEvaluated(target, instance, location.length > 0 ? location[location.length - 1] : undefined);
  }
  return result;
}

function AssertionPropertyTypeStrictEvaluate_fast(instruction, instance, depth, template, evaluator) {
  if (!isObject(instance)) return true;
  const target = resolveInstance(instance, instruction[2]);
  if (target === undefined) return true;
  const result = effectiveTypeStrictReal(target) === instruction[5];
  if (result && evaluator.trackMode) {
    const location = instruction[2];
    evaluator.markEvaluated(target, instance, location.length > 0 ? location[location.length - 1] : undefined);
  }
  return result;
}

function AssertionPropertyTypeStrictAny_fast(instruction, instance, depth, template, evaluator) {
  if (!isObject(instance)) return true;
  const target = resolveInstance(instance, instruction[2]);
  if (target === undefined) return true;
  return typeSetTest(instruction[5], effectiveTypeStrictReal(target));
}

function AssertionPropertyTypeStrictAnyEvaluate_fast(instruction, instance, depth, template, evaluator) {
  if (!isObject(instance)) return true;
  const target = resolveInstance(instance, instruction[2]);
  if (target === undefined) return true;
  const result = typeSetTest(instruction[5], effectiveTypeStrictReal(target));
  if (result && evaluator.trackMode) {
    const location = instruction[2];
    evaluator.markEvaluated(target, instance, location.length > 0 ? location[location.length - 1] : undefined);
  }
  return result;
}

function AssertionArrayPrefix_fast(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!Array.isArray(target)) return true;
  if (target.length === 0) return true;
  const children = instruction[6];
  const prefixes = children.length - 1;
  const pointer = target.length === prefixes ? prefixes : Math.min(target.length, prefixes) - 1;
  const entry = children[pointer];
  const entryChildren = entry[6];
  if (entryChildren) {
    for (let index = 0; index < entryChildren.length; index++) {
      if (!evaluateInstruction(entryChildren[index], target, depth + 1, template, evaluator)) return false;
    }
  }
  return true;
}

function AssertionArrayPrefixEvaluate_fast(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!Array.isArray(target)) return true;
  if (target.length === 0) return true;
  const children = instruction[6];
  const prefixes = children.length - 1;
  const pointer = target.length === prefixes ? prefixes : Math.min(target.length, prefixes) - 1;
  const entry = children[pointer];
  const entryChildren = entry[6];
  if (entryChildren) {
    for (let index = 0; index < entryChildren.length; index++) {
      if (!evaluateInstruction(entryChildren[index], target, depth + 1, template, evaluator)) return false;
    }
  }
  if (evaluator.trackMode) {
    if (target.length === prefixes) {
      evaluator.markEvaluated(target);
    } else {
      for (let cursor = 0; cursor <= pointer; cursor++) {
        evaluator.markEvaluated(target[cursor], target, cursor);
      }
    }
  }
  return true;
}

function AssertionObjectPropertiesSimple_fast(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return false;
  const value = instruction[5];
  const children = instruction[6];
  for (let index = 0; index < value.length; index++) {
    const entry = value[index];
    const name = entry[0];
    const required = entry[2];
    if (!Object.hasOwn(target, name)) {
      if (required) return false;
      continue;
    }
    if (index < children.length) {
      if (!evaluateInstructionFast(children[index], target[name], depth + 1, template, evaluator)) return false;
    }
  }
  return true;
}

function AnnotationEmit_fast() { return true; }
function AnnotationToParent_fast() { return true; }
function AnnotationBasenameToParent_fast() { return true; }

function Evaluate_fast(instruction, instance, depth, template, evaluator) {
  if (evaluator.trackMode) {
    const target = resolveInstance(instance, instruction[2]);
    evaluator.markEvaluated(target);
  }
  return true;
}

function LogicalNotEvaluate_fast(instruction, instance, depth, template, evaluator) {
  const relInstance = instruction[2];
  const target = relInstance.length === 0 ? instance : resolveInstance(instance, relInstance);
  const children = instruction[6];
  let result = false;
  if (children) {
    for (let index = 0; index < children.length; index++) {
      if (!evaluateInstruction(children[index], target, depth + 1, template, evaluator)) {
        result = true;
        break;
      }
    }
  }
  if (evaluator.trackMode) evaluator.unevaluate();
  return result;
}

function LogicalWhenArraySizeGreater_fast(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!Array.isArray(target) || target.length <= instruction[5]) return true;
  const children = instruction[6];
  if (children) {
    for (let index = 0; index < children.length; index++) {
      if (!evaluateInstruction(children[index], target, depth + 1, template, evaluator)) return false;
    }
  }
  return true;
}

function LoopPropertiesUnevaluated_fast(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return true;
  if (evaluator.trackMode && evaluator.isEvaluated(target)) return true;
  const children = instruction[6];
  for (const key in target) {
    if (evaluator.trackMode && evaluator.isEvaluated(target[key], target, key)) continue;
    if (children) {
      for (let childIndex = 0; childIndex < children.length; childIndex++) {
        if (!evaluateInstruction(children[childIndex], target[key], depth + 1, template, evaluator)) return false;
      }
    }
  }
  if (evaluator.trackMode) evaluator.markEvaluated(target);
  return true;
}

function LoopPropertiesUnevaluatedExcept_fast(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return true;
  if (evaluator.trackMode && evaluator.isEvaluated(target)) return true;
  const filter = instruction[5];
  const filterStrings = filter[0];
  const filterPrefixes = filter[1];
  const filterRegexes = filter[2];
  const children = instruction[6];
  for (const key in target) {
    if (filterStrings.has(key)) continue;
    let matched = false;
    for (let index = 0; index < filterPrefixes.length; index++) {
      if (key.startsWith(filterPrefixes[index])) { matched = true; break; }
    }
    if (matched) continue;
    for (let index = 0; index < filterRegexes.length; index++) {
      filterRegexes[index].lastIndex = 0;
      if (filterRegexes[index].test(key)) { matched = true; break; }
    }
    if (matched) continue;
    if (evaluator.trackMode && evaluator.isEvaluated(target[key], target, key)) continue;
    if (children) {
      for (let childIndex = 0; childIndex < children.length; childIndex++) {
        if (!evaluateInstruction(children[childIndex], target[key], depth + 1, template, evaluator)) return false;
      }
    }
  }
  if (evaluator.trackMode) evaluator.markEvaluated(target);
  return true;
}

function LoopPropertiesEvaluate_fast(instruction, instance, depth, template, evaluator) {
  const relInstance = instruction[2];
  const target = relInstance.length === 0 ? instance : resolveInstance(instance, relInstance);
  if (!isObject(target)) return true;
  const children = instruction[6];
  for (const key in target) {
    evaluator.propertyParent = target;
    evaluator.propertyKey = key;
    if (children) {
      for (let childIndex = 0; childIndex < children.length; childIndex++) {
        if (!evaluateInstruction(children[childIndex], target[key], depth + 1, template, evaluator)) {
          evaluator.propertyParent = undefined;
          evaluator.propertyKey = undefined;
          return false;
        }
      }
    }
  }
  evaluator.propertyParent = undefined;
  evaluator.propertyKey = undefined;
  if (evaluator.trackMode) evaluator.markEvaluated(target);
  return true;
}

function LoopPropertiesRegex_fast(instruction, instance, depth, template, evaluator) {
  const relInstance = instruction[2];
  const target = relInstance.length === 0 ? instance : resolveInstance(instance, relInstance);
  if (!isObject(target)) return true;
  const regex = instruction[5];
  const children = instruction[6];
  for (const key in target) {
    regex.lastIndex = 0;
    if (!regex.test(key)) continue;
    evaluator.propertyParent = target;
    evaluator.propertyKey = key;
    if (children) {
      for (let childIndex = 0; childIndex < children.length; childIndex++) {
        if (!evaluateInstruction(children[childIndex], target[key], depth + 1, template, evaluator)) {
          evaluator.propertyParent = undefined;
          evaluator.propertyKey = undefined;
          return false;
        }
      }
    }
  }
  evaluator.propertyParent = undefined;
  evaluator.propertyKey = undefined;
  return true;
}

function LoopPropertiesRegexClosed_fast(instruction, instance, depth, template, evaluator) {
  const relInstance = instruction[2];
  const target = relInstance.length === 0 ? instance : resolveInstance(instance, relInstance);
  if (!isObject(target)) return true;
  const regex = instruction[5];
  const children = instruction[6];
  for (const key in target) {
    regex.lastIndex = 0;
    if (!regex.test(key)) return false;
    evaluator.propertyParent = target;
    evaluator.propertyKey = key;
    if (children) {
      for (let childIndex = 0; childIndex < children.length; childIndex++) {
        if (!evaluateInstruction(children[childIndex], target[key], depth + 1, template, evaluator)) {
          evaluator.propertyParent = undefined;
          evaluator.propertyKey = undefined;
          return false;
        }
      }
    }
  }
  evaluator.propertyParent = undefined;
  evaluator.propertyKey = undefined;
  return true;
}

function LoopPropertiesStartsWith_fast(instruction, instance, depth, template, evaluator) {
  const relInstance = instruction[2];
  const target = relInstance.length === 0 ? instance : resolveInstance(instance, relInstance);
  if (!isObject(target)) return true;
  const prefix = instruction[5];
  const children = instruction[6];
  for (const key in target) {
    if (!key.startsWith(prefix)) continue;
    evaluator.propertyParent = target;
    evaluator.propertyKey = key;
    if (children) {
      for (let childIndex = 0; childIndex < children.length; childIndex++) {
        if (!evaluateInstruction(children[childIndex], target[key], depth + 1, template, evaluator)) {
          evaluator.propertyParent = undefined;
          evaluator.propertyKey = undefined;
          return false;
        }
      }
    }
  }
  evaluator.propertyParent = undefined;
  evaluator.propertyKey = undefined;
  return true;
}

function LoopPropertiesType_fast(instruction, instance, depth, template, evaluator) {
  const relInstance = instruction[2];
  const target = relInstance.length === 0 ? instance : resolveInstance(instance, relInstance);
  if (!isObject(target)) return true;
  const expected = instruction[5];
  for (const key in target) {
    const actual = jsonTypeOf(target[key]);
    if (actual !== expected && !(expected === Type.Integer && isIntegral(target[key]))) return false;
  }
  return true;
}

function LoopPropertiesTypeEvaluate_fast(instruction, instance, depth, template, evaluator) {
  const relInstance = instruction[2];
  const target = relInstance.length === 0 ? instance : resolveInstance(instance, relInstance);
  if (!isObject(target)) return true;
  const expected = instruction[5];
  for (const key in target) {
    const actual = jsonTypeOf(target[key]);
    if (actual !== expected && !(expected === Type.Integer && isIntegral(target[key]))) return false;
  }
  if (evaluator.trackMode) evaluator.markEvaluated(target);
  return true;
}

function LoopPropertiesExactlyTypeStrict_fast(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return false;
  const value = instruction[5];
  let count = 0;
  for (const key in target) {
    count++;
    if (effectiveTypeStrictReal(target[key]) !== value[0]) return false;
  }
  return count === value[1].length;
}

function LoopPropertiesExactlyTypeStrictHash_fast(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return false;
  const value = instruction[5];
  const entries = value[1][0];
  const expectedCount = entries.length;
  let count = 0;
  for (const key in target) {
    count++;
    if (effectiveTypeStrictReal(target[key]) !== value[0]) return false;
  }
  if (count !== expectedCount) return false;
  for (let index = 0; index < expectedCount; index++) {
    if (!Object.hasOwn(target, entries[index][1])) return false;
  }
  return true;
}

function LoopPropertiesTypeStrict_fast(instruction, instance, depth, template, evaluator) {
  const relInstance = instruction[2];
  const target = relInstance.length === 0 ? instance : resolveInstance(instance, relInstance);
  if (!isObject(target)) return true;
  const expected = instruction[5];
  for (const key in target) {
    if (effectiveTypeStrictReal(target[key]) !== expected) return false;
  }
  return true;
}

function LoopPropertiesTypeStrictEvaluate_fast(instruction, instance, depth, template, evaluator) {
  const relInstance = instruction[2];
  const target = relInstance.length === 0 ? instance : resolveInstance(instance, relInstance);
  if (!isObject(target)) return true;
  const expected = instruction[5];
  for (const key in target) {
    if (effectiveTypeStrictReal(target[key]) !== expected) return false;
  }
  if (evaluator.trackMode) evaluator.markEvaluated(target);
  return true;
}

function LoopPropertiesTypeStrictAny_fast(instruction, instance, depth, template, evaluator) {
  const relInstance = instruction[2];
  const target = relInstance.length === 0 ? instance : resolveInstance(instance, relInstance);
  if (!isObject(target)) return true;
  const bitmask = instruction[5];
  for (const key in target) {
    if (!typeSetTest(bitmask, effectiveTypeStrictReal(target[key]))) return false;
  }
  return true;
}

function LoopPropertiesTypeStrictAnyEvaluate_fast(instruction, instance, depth, template, evaluator) {
  const relInstance = instruction[2];
  const target = relInstance.length === 0 ? instance : resolveInstance(instance, relInstance);
  if (!isObject(target)) return true;
  const bitmask = instruction[5];
  for (const key in target) {
    if (!typeSetTest(bitmask, effectiveTypeStrictReal(target[key]))) return false;
  }
  if (evaluator.trackMode) evaluator.markEvaluated(target);
  return true;
}

function LoopKeys_fast(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!isObject(target)) return true;
  const children = instruction[6];
  for (const key in target) {
    const previousPropertyTarget = evaluator.propertyTarget;
    evaluator.propertyTarget = key;
    if (children) {
      for (let childIndex = 0; childIndex < children.length; childIndex++) {
        if (!evaluateInstruction(children[childIndex], null, depth + 1, template, evaluator)) {
          evaluator.propertyTarget = previousPropertyTarget;
          return false;
        }
      }
    }
    evaluator.propertyTarget = previousPropertyTarget;
  }
  return true;
}

function LoopItemsFrom_fast(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  const startIndex = instruction[5];
  if (!Array.isArray(target) || startIndex >= target.length) return true;
  const children = instruction[6];
  for (let index = startIndex; index < target.length; index++) {
    if (children) {
      for (let childIndex = 0; childIndex < children.length; childIndex++) {
        if (!evaluateInstruction(children[childIndex], target[index], depth + 1, template, evaluator)) return false;
      }
    }
  }
  return true;
}

function LoopItemsUnevaluated_fast(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!Array.isArray(target)) return true;
  if (evaluator.trackMode && evaluator.isEvaluated(target)) return true;
  const children = instruction[6];
  for (let index = 0; index < target.length; index++) {
    if (evaluator.trackMode && evaluator.isEvaluated(target[index], target, index)) continue;
    if (children) {
      for (let childIndex = 0; childIndex < children.length; childIndex++) {
        if (!evaluateInstruction(children[childIndex], target[index], depth + 1, template, evaluator)) return false;
      }
    }
  }
  if (evaluator.trackMode) evaluator.markEvaluated(target);
  return true;
}

function LoopItemsPropertiesExactlyTypeStrictHash_fast(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!Array.isArray(target)) return false;
  const expectedType = instruction[5][0];
  const entries = instruction[5][1][0];
  const expectedCount = entries.length;
  for (let index = 0; index < target.length; index++) {
    const item = target[index];
    if (!isObject(item)) return false;
    let count = 0;
    for (const key in item) {
      count++;
      if (effectiveTypeStrictReal(item[key]) !== expectedType) return false;
    }
    if (count !== expectedCount) return false;
    for (let entry = 0; entry < expectedCount; entry++) {
      if (!Object.hasOwn(item, entries[entry][1])) return false;
    }
  }
  return true;
}

function ControlDynamicAnchorJump_fast(instruction, instance, depth, template, evaluator) {
  const resolved = resolveInstance(instance, instruction[2]);
  const anchor = instruction[5];
  if (!evaluator.resources) return false;
  const anchors = template[5];
  for (let index = 0; index < evaluator.resources.length; index++) {
    const jumpTarget = anchors.get(evaluator.resources[index] + ':' + anchor);
    if (jumpTarget !== undefined) {
      for (let childIndex = 0; childIndex < jumpTarget.length; childIndex++) {
        if (!evaluateInstruction(jumpTarget[childIndex], resolved, depth + 1, template, evaluator)) return false;
      }
      return true;
    }
  }
  return false;
}

function LoopItemsIntegerBounded_fast(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  if (!Array.isArray(target) || target.length === 0) return true;
  const minimum = instruction[5][0];
  const maximum = instruction[5][1];
  for (let index = 0; index < target.length; index++) {
    const element = target[index];
    const elementType = typeof element;
    if ((elementType !== 'number' && elementType !== 'bigint') || element < minimum || element > maximum) return false;
  }
  return true;
}

function LoopItemsIntegerBoundedSized_fast(instruction, instance, depth, template, evaluator) {
  const value = instruction[5];
  const minimum = value[0][0];
  const maximum = value[0][1];
  const minimumSize = value[1][0];
  const target = resolveInstance(instance, instruction[2]);
  if (!Array.isArray(target) || target.length < minimumSize) return false;
  for (let index = 0; index < target.length; index++) {
    const element = target[index];
    const elementType = typeof element;
    if ((elementType !== 'number' && elementType !== 'bigint') || element < minimum || element > maximum) return false;
  }
  return true;
}

function AssertionTypeIntegerBounded_fast(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  const range = instruction[5];
  return (typeof target === 'bigint' || Number.isInteger(target)) && target >= range[0] && target <= range[1];
}

function AssertionTypeIntegerBoundedStrict_fast(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  const range = instruction[5];
  return (typeof target === 'bigint' || Number.isInteger(target)) && target >= range[0] && target <= range[1];
}

function AssertionTypeIntegerLowerBound_fast(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  const range = instruction[5];
  return (typeof target === 'bigint' || Number.isInteger(target)) && target >= range[0];
}

function AssertionTypeIntegerLowerBoundStrict_fast(instruction, instance, depth, template, evaluator) {
  const target = resolveInstance(instance, instruction[2]);
  const range = instruction[5];
  return (typeof target === 'bigint' || Number.isInteger(target)) && target >= range[0];
}

const fastHandlers = handlers.slice();
fastHandlers[15] = AssertionTypeArrayBounded_fast;
fastHandlers[86] = LoopItemsTypeStrictAny_fast;
fastHandlers[42] = AssertionPropertyTypeStrict_fast;
fastHandlers[11] = AssertionTypeStrict_fast;
fastHandlers[4] = AssertionDefinesAllStrict_fast;
fastHandlers[26] = AssertionEqual_fast;
fastHandlers[64] = LoopPropertiesMatch_fast;
fastHandlers[55] = LogicalOr_fast;
fastHandlers[98] = ControlJump_fast;
fastHandlers[28] = AssertionEqualsAnyStringHash_fast;
fastHandlers[57] = LogicalXor_fast;
fastHandlers[2] = AssertionDefinesStrict_fast;
fastHandlers[81] = LoopItems_fast;
fastHandlers[65] = LoopPropertiesMatchClosed_fast;
fastHandlers[13] = AssertionTypeStringBounded_fast;
fastHandlers[56] = LogicalAnd_fast;
fastHandlers[8] = AssertionPropertyDependencies_fast;
fastHandlers[10] = AssertionTypeAny_fast;
fastHandlers[58] = LogicalCondition_fast;
fastHandlers[71] = LoopPropertiesExcept_fast;
fastHandlers[19] = AssertionRegex_fast;
fastHandlers[66] = LoopProperties_fast;
fastHandlers[1] = AssertionDefines_fast;
fastHandlers[59] = LogicalWhenType_fast;
fastHandlers[60] = LogicalWhenDefines_fast;
fastHandlers[0] = AssertionFail_fast;
fastHandlers[91] = LoopContains_fast;
fastHandlers[53] = LogicalNot_fast;
fastHandlers[84] = LoopItemsType_fast;
fastHandlers[85] = LoopItemsTypeStrict_fast;
fastHandlers[27] = AssertionEqualsAny_fast;
fastHandlers[3] = AssertionDefinesAll_fast;
fastHandlers[5] = AssertionDefinesExactly_fast;
fastHandlers[6] = AssertionDefinesExactlyStrict_fast;
fastHandlers[7] = AssertionDefinesExactlyStrictHash3_fast;
fastHandlers[9] = AssertionType_fast;
fastHandlers[12] = AssertionTypeStrictAny_fast;
fastHandlers[14] = AssertionTypeStringUpper_fast;
fastHandlers[16] = AssertionTypeArrayUpper_fast;
fastHandlers[17] = AssertionTypeObjectBounded_fast;
fastHandlers[18] = AssertionTypeObjectUpper_fast;
fastHandlers[20] = AssertionStringSizeLess_fast;
fastHandlers[21] = AssertionStringSizeGreater_fast;
fastHandlers[22] = AssertionArraySizeLess_fast;
fastHandlers[23] = AssertionArraySizeGreater_fast;
fastHandlers[24] = AssertionObjectSizeLess_fast;
fastHandlers[25] = AssertionObjectSizeGreater_fast;
fastHandlers[29] = AssertionGreaterEqual_fast;
fastHandlers[30] = AssertionLessEqual_fast;
fastHandlers[31] = AssertionGreater_fast;
fastHandlers[32] = AssertionLess_fast;
fastHandlers[33] = AssertionUnique_fast;
fastHandlers[34] = AssertionDivisible_fast;
fastHandlers[35] = AssertionTypeIntegerBounded_fast;
fastHandlers[36] = AssertionTypeIntegerBoundedStrict_fast;
fastHandlers[37] = AssertionTypeIntegerLowerBound_fast;
fastHandlers[38] = AssertionTypeIntegerLowerBoundStrict_fast;
fastHandlers[39] = AssertionStringType_fast;
fastHandlers[40] = AssertionPropertyType_fast;
fastHandlers[41] = AssertionPropertyTypeEvaluate_fast;
fastHandlers[43] = AssertionPropertyTypeStrictEvaluate_fast;
fastHandlers[44] = AssertionPropertyTypeStrictAny_fast;
fastHandlers[45] = AssertionPropertyTypeStrictAnyEvaluate_fast;
fastHandlers[46] = AssertionArrayPrefix_fast;
fastHandlers[47] = AssertionArrayPrefixEvaluate_fast;
fastHandlers[48] = AssertionObjectPropertiesSimple_fast;
fastHandlers[49] = AnnotationEmit_fast;
fastHandlers[50] = AnnotationToParent_fast;
fastHandlers[51] = AnnotationBasenameToParent_fast;
fastHandlers[52] = Evaluate_fast;
fastHandlers[54] = LogicalNotEvaluate_fast;
fastHandlers[61] = LogicalWhenArraySizeGreater_fast;
fastHandlers[62] = LoopPropertiesUnevaluated_fast;
fastHandlers[63] = LoopPropertiesUnevaluatedExcept_fast;
fastHandlers[67] = LoopPropertiesEvaluate_fast;
fastHandlers[68] = LoopPropertiesRegex_fast;
fastHandlers[69] = LoopPropertiesRegexClosed_fast;
fastHandlers[70] = LoopPropertiesStartsWith_fast;
fastHandlers[72] = LoopPropertiesType_fast;
fastHandlers[73] = LoopPropertiesTypeEvaluate_fast;
fastHandlers[74] = LoopPropertiesExactlyTypeStrict_fast;
fastHandlers[75] = LoopPropertiesExactlyTypeStrictHash_fast;
fastHandlers[76] = LoopPropertiesTypeStrict_fast;
fastHandlers[77] = LoopPropertiesTypeStrictEvaluate_fast;
fastHandlers[78] = LoopPropertiesTypeStrictAny_fast;
fastHandlers[79] = LoopPropertiesTypeStrictAnyEvaluate_fast;
fastHandlers[80] = LoopKeys_fast;
fastHandlers[82] = LoopItemsFrom_fast;
fastHandlers[83] = LoopItemsUnevaluated_fast;
fastHandlers[87] = LoopItemsPropertiesExactlyTypeStrictHash_fast;
fastHandlers[88] = LoopItemsPropertiesExactlyTypeStrictHash_fast;
fastHandlers[89] = LoopItemsIntegerBounded_fast;
fastHandlers[90] = LoopItemsIntegerBoundedSized_fast;
fastHandlers[97] = ControlDynamicAnchorJump_fast;

export { Blaze };
