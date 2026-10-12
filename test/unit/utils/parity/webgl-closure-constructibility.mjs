// Fixed-upstream declarations are ordinary strict functions, not methods/arrows.
const encode=v=>v===undefined?'undefined':typeof v==='function'?'function':typeof v==='number'?
    Number.isNaN(v)?'NaN':!Number.isFinite(v)?v<0?'-Infinity':'Infinity':Object.is(v,-0)?'-0':v:
    Array.isArray(v)?v.map(encode):v&&typeof v==='object'?Object.fromEntries(Object.keys(v).map(k=>[k,encode(v[k])])):v;
const flags=(v,k)=>{const d=Object.getOwnPropertyDescriptor(v,k);return [!!d,!!d?.writable,!!d?.enumerable,!!d?.configurable];};
const metadata=fn=>({name:fn.name,length:fn.length,own:Object.getOwnPropertyNames(fn),nameFlags:flags(fn,'name'),lengthFlags:flags(fn,'length'),
    prototypeFlags:flags(fn,'prototype'),prototypeOwn:fn.prototype&&typeof fn.prototype==='object'?Object.getOwnPropertyNames(fn.prototype):null,
    constructorFlags:fn.prototype&&typeof fn.prototype==='object'?flags(fn.prototype,'constructor'):null,
    constructorIdentity:fn.prototype?.constructor===fn,functionPrototype:Object.getPrototypeOf(fn)===Function.prototype});
export function closureCases(group,constructors) {
    const {WebGLInfo,WebGLBufferRenderer,WebGLIndexedBufferRenderer,WebGLProperties}=constructors;
    function fixture(){
        const f={trace:[],gl:{TRIANGLES:4,LINES:1,LINE_STRIP:3,LINE_LOOP:2,POINTS:0},extension:{},extensions:{},info:{},key:{},cached:undefined};
        if(group==='info'){f.object=new WebGLInfo(f.gl);f.object.render.calls=7;f.object.render.triangles=5;return f;}
        if(group==='properties'){f.object=new WebGLProperties();f.cached=f.object.get(f.key);f.cached.before=1;return f;}
        const record=(name,receiver)=>function(...args){f.trace.push([name,this===receiver,...args]);};
        for(const n of ['drawArrays','drawArraysInstanced','drawElements','drawElementsInstanced'])f.gl[n]=record(n,f.gl);
        for(const n of ['multiDrawArraysWEBGL','multiDrawElementsWEBGL'])f.extension[n]=record(n,f.extension);
        f.extensions.get=function(name){f.trace.push(['get',this===f.extensions,name]);return f.extension;};f.info.update=record('info',f.info);
        f.object=new (group==='indexed'?WebGLIndexedBufferRenderer:WebGLBufferRenderer)(f.gl,f.extensions,f.info);
        f.object.setMode(4);if(group==='indexed')f.object.setIndex({type:5123,bytesPerElement:2});return f;
    }
    const args=(f,m)=>group==='info'?m==='reset'?[]:[12,4,2]:group==='properties'?m==='dispose'?[]:m==='update'?[f.key,'x',42]:[f.key]:
        m==='setMode'?[1]:m==='setIndex'?[{type:5125,bytesPerElement:4}]:m==='render'?[2,6]:m==='renderInstances'?[2,6,3]:[[2,4],[6,9],2];
    const state=f=>group==='info'?encode(f.object.render):group==='properties'?{old:encode(f.cached),has:encode(f.object.has(f.key))}:null;
    const isObject=v=>v!==null&&['object','function'].includes(typeof v);
    const result=(v,fn,target,custom,cached)=>({object:isObject(v),cached:isObject(v)&&v===cached,
        functionPrototype:isObject(v)&&Object.getPrototypeOf(v)===fn.prototype,
        targetPrototype:isObject(v)&&isObject(target)&&Object.getPrototypeOf(v)===target.prototype,
        customPrototype:isObject(v)&&Object.getPrototypeOf(v)===custom,objectPrototype:isObject(v)&&Object.getPrototypeOf(v)===Object.prototype,
        own:isObject(v)?Object.getOwnPropertyNames(v):encode(v)});
    const out=[],methods=group==='info'?['reset','update']:group==='properties'?['has','get','remove','update','dispose']:
        group==='indexed'?['setMode','setIndex','render','renderInstances','renderMultiDraw']:['setMode','render','renderInstances','renderMultiDraw'];
    for(const method of methods)for(let scenario=0;scenario<11;scenario++){
        const f=fixture(),fn=f.object[method],meta=metadata(fn),targetCalls=[],custom={marker:17};function Target(){targetCalls.push(['targetBody']);}
        let argv=args(f,method),newTarget=fn;
        if(scenario===1||scenario===9){Target.prototype=custom;if(scenario===9)Object.preventExtensions(custom);newTarget=Target;}
        if(scenario===2||scenario===10){Target.prototype=scenario===2?17:null;newTarget=Target;}
        if(scenario===3)fn.prototype=17;if(scenario===4)fn.prototype=custom;
        if(scenario===5)newTarget=()=>{};if(scenario===6)newTarget={};if(scenario===8)argv=[];
        let observed;try{const v=scenario===7?fn.apply(null,argv):Reflect.construct(fn,argv,newTarget);observed=result(v,fn,newTarget,custom,f.cached);}catch(e){observed={error:e.name};}
        if(['buffer','indexed'].includes(group)&&['setMode','setIndex'].includes(method))f.object.render(2,6);
        out.push({kind:'closure-construction',group,method,scenario,metadata:meta,result:observed,state:state(f),trace:encode(f.trace),targetCalls:encode(targetCalls)});
    }
    for(let scenario=0;scenario<(group==='buffer'?1:2);scenario++){
        const f=fixture();let method,argv;
        if(group==='info'){
            method=scenario===0?'update':'reset';argv=[12,4,2];
            if(scenario===0)argv[0]={[Symbol.toPrimitive](hint){f.trace.push(['convert',hint]);throw RangeError('conversion');}};
            else Object.defineProperty(f.object.render,'triangles',{set(v){f.trace.push(['setter',v]);throw RangeError('setter');},configurable:true});
        }else if(group==='properties'){
            method=scenario===0?'get':'update';argv=scenario===0?[17]:[f.key,'x',42];
            if(scenario===1){f.object.remove(f.key);argv[1]={[Symbol.toPrimitive](){f.trace.push(['unexpected-key']);return 'x';}};}
        }else{
            method=scenario===0?'render':'setIndex';argv=scenario===0?[2,6]:[{type:5125,bytesPerElement:4}];
            if(scenario===0)f.gl[group==='indexed'?'drawElements':'drawArrays']=function(){f.trace.push(['draw-throw']);throw RangeError('draw');};
            else Object.defineProperty(argv[0],'bytesPerElement',{get(){f.trace.push(['index-get-throw']);throw RangeError('index');},configurable:true});
        }
        let observed;try{observed=encode(Reflect.construct(f.object[method],argv));}catch(e){observed={error:e.name};}
        if(group==='indexed'&&scenario===1)f.object.render(2,6);
        out.push({kind:'closure-exception',group,scenario,result:observed,state:state(f),trace:encode(f.trace)});
    }
    if(group==='properties')for(const method of ['has','get'])for(let scenario=0;scenario<8;scenario++){
        const f=fixture(),fn=f.object[method],saved=Object.getOwnPropertyDescriptor(WeakMap.prototype,method);
        const returned=[true,{},[],function Returned(){},null,17,'text',undefined][scenario];
        try{
            Object.defineProperty(WeakMap.prototype,method,{value:function(){f.trace.push(['override',this instanceof WeakMap]);return returned;}});
            const custom={};function Target(){}Target.prototype=custom;
            const value=Reflect.construct(fn,[f.key],Target);
            out.push({kind:'closure-return',method,scenario,returnedIdentity:value===returned,object:isObject(value),customPrototype:Object.getPrototypeOf(value)===custom,
                originalCache:value===f.cached,own:Object.getOwnPropertyNames(value),trace:encode(f.trace)});
        }finally{Object.defineProperty(WeakMap.prototype,method,saved);}
    }
    return out;
}
