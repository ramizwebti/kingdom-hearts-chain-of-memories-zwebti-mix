/* Exercise the generated standalone catalog's actual client code with a
 * minimal DOM contract. This checks behavior, not browser pixel layout. */
const fs=require('node:fs');
const vm=require('node:vm');
const assert=require('node:assert/strict');
const page=fs.readFileSync(process.argv[2]||'build/native-catalog-v010/native_catalog.html','utf8');
const payload=page.match(/<script id="catalog" type="application\/json">([\s\S]*?)<\/script>/)[1];
const code=page.match(/<script>([\s\S]*?)<\/script>/)[1];
const nodes={};
for(const id of ['catalog','counts','scope','coverage','search','kind','group','summary','previous','next','rows','detail','heading','json','close'])
  nodes[id]={value:'',innerHTML:'',textContent:'',hidden:id==='detail',children:[],append(child){this.children.push(child)}};
nodes.catalog.textContent=payload;
const document={getElementById:id=>nodes[id],createElement:()=>({value:'',textContent:''})};
vm.runInNewContext(code,{document,console},{timeout:30000});
assert(nodes.kind.children.some(row=>row.value==='native-attack-definition'));
assert(nodes.group.children.length>100);
nodes.kind.value='native-attack-definition';nodes.kind.oninput();
assert(nodes.summary.textContent.startsWith('330 matching entries'));
assert.equal((nodes.rows.innerHTML.match(/<tr /g)||[]).length,100);
nodes.next.onclick();
assert(nodes.rows.innerHTML.includes('sBattleAttackDefs[100]'));
nodes.rows.onclick({target:{closest:()=>({dataset:{i:'100'}})}});
assert.equal(nodes.detail.hidden,false);
assert.equal(JSON.parse(nodes.json.textContent).attack_id,100);
nodes.close.onclick();assert.equal(nodes.detail.hidden,true);
nodes.kind.value='native-move-id';nodes.search.value='MOVE_FIRE_WALL';nodes.search.oninput();
assert(nodes.summary.textContent.startsWith('1 matching entries'));
nodes.rows.onclick({target:{closest:()=>({dataset:{i:'0'}})}});
assert.equal(JSON.parse(nodes.json.textContent).connected_controller_route_member,false);
nodes.kind.value='sprite-animation';nodes.search.value='gSor1ll';nodes.search.oninput();
assert(!nodes.summary.textContent.startsWith('0 matching entries'));
console.log('PASS: standalone catalog client filters native attacks/moves/animations, paginates330 IDs, opens exact source metadata and closes details. Pixel layout is not asserted.');
