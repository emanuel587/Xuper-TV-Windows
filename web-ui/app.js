(() => {
  const $ = (s, root = document) => root.querySelector(s);
  const $$ = (s, root = document) => [...root.querySelectorAll(s)];
  const railRoot = $("#dynamic-shelves"), quickSection = $("#quick-section"), quickGrid = $("#quick-grid");
  const toast = $("#toast"), navItems = $$(".nav-item[data-page]");
  const searchDialog = $("#search-dialog"), detailsDialog = $("#details-dialog");
  let activePage = "Inicio", items = [], selectedIndex = 0, toastTimer = 0;

  const icons = {
    live:'<rect x="3" y="5" width="18" height="14" rx="2"/><path d="m8 2 4 3 4-3M10 9l5 3-5 3z"/>',
    movie:'<rect x="3" y="4" width="18" height="16" rx="2"/><path d="M7 4v16M17 4v16M3 9h4m-4 6h4m10-6h4m-4 6h4"/>',
    series:'<rect x="4" y="3" width="16" height="18" rx="2"/><path d="M8 7h8M8 11h8M8 15h5"/>',
    category:'<rect x="3" y="4" width="18" height="16" rx="2"/><path d="M8 8h8M8 12h8M8 16h5"/>',
    favorite:'<path d="m12 3 2.8 5.7 6.2.9-4.5 4.4 1.1 6.2-5.6-3-5.6 3 1.1-6.2L3 9.6l6.2-.9z"/>'
  };
  const svg = (path) => { const el = document.createElementNS("http://www.w3.org/2000/svg","svg"); el.setAttribute("viewBox","0 0 24 24"); el.innerHTML = path; return el; };
  const notify = (message) => { toast.textContent=message; toast.classList.add("show"); clearTimeout(toastTimer); toastTimer=setTimeout(()=>toast.classList.remove("show"),2400); };
  const send = (type,payload={}) => {
    const native = {
      navigate:()=>window.xuperNavigate?.(payload.page||activePage),
      "play-selected":()=>window.xuperPlay?.(),
      favorite:()=>window.xuperFavorite?.(),
      "search-query":()=>window.xuperSearch?.(payload.query||""),
      "select-item":()=>window.xuperSelectItem?.(Number(payload.index||0)),
      back:()=>window.xuperBack?.(),
      "show-all":()=>window.xuperNavigate?.(activePage)
    };
    const bound = {
      navigate:window.xuperNavigate, "play-selected":window.xuperPlay, favorite:window.xuperFavorite,
      "search-query":window.xuperSearch, "select-item":window.xuperSelectItem, back:window.xuperBack
    };
    if(typeof bound[type] === "function") { native[type]?.(); return; }
    if(window.chrome?.webview) { window.chrome.webview.postMessage({type,...payload}); return; }
    notify("No se pudo conectar con el motor de Xuper TV.");
  };
  const titleCase = (s) => (s||"").toString().replace(/\s+/g," ").trim();
  const pageCopy = {
    "Inicio":["TU ESPACIO DE ENTRETENIMIENTO","¿Qué quieres ver hoy?","Todo lo que te gusta, reunido en un solo lugar."],
    "TV en vivo":["TELEVISIÓN EN DIRECTO","Tu televisión, en un solo lugar.","Explora las categorías y encuentra tu canal."],
    "Películas":["UNA HISTORIA PARA CADA MOMENTO","Noche de película.","Descubre títulos para todos tus momentos."],
    "Series":["SIGUE DONDE TE QUEDASTE","Tu próxima serie favorita.","Encuentra una historia para continuar."],
    "Favoritos":["TU COLECCIÓN PERSONAL","Tus favoritos.","Tus canales guardados en un mismo lugar."],
    "Apoyar":["GRACIAS POR ESTAR AQUÍ","Apoya el proyecto.","Ayúdanos a seguir mejorando Xuper TV."]
  };
  const setPage = (page,notifyNative=true) => {
    activePage=page;
    navItems.forEach(b=>{const on=b.dataset.page===page;b.classList.toggle("active",on);if(on)b.setAttribute("aria-current","page");else b.removeAttribute("aria-current");});
    const copy=pageCopy[page]||["DESCUBRE MÁS","Explora el catálogo.", "Encuentra algo que te guste."];
    $("#breadcrumb").innerHTML="XUPER TV <b>/</b> "+page.toLocaleUpperCase("es-MX");
    $("#page-eyebrow").textContent=copy[0];$("#page-title").textContent=copy[1];$("#page-subtitle").textContent=copy[2];
    if(notifyNative)send("navigate",{page});
  };
  const setStatus = (message) => { $("#service-status").textContent=message||"Listo"; };
  navItems.forEach(b=>b.addEventListener("click",()=>setPage(b.dataset.page)));
  const openSearch=()=>{if(!searchDialog.open){searchDialog.showModal();$("#search-query").focus();}};
  $("#search").addEventListener("click",openSearch);
  $("#search-form").addEventListener("submit",e=>{e.preventDefault();const query=$("#search-query").value.trim();if(!query)return;searchDialog.close();send("search-query",{query});});
  $("#watch").addEventListener("click",()=>{if(items.length)send("play-selected");else notify("Primero conecta el servicio para cargar tu catálogo.");});
  $("#favorite").addEventListener("click",()=>send("favorite"));
  $("#more").addEventListener("click",()=>showDetails(items[selectedIndex]));
  $("#details-watch").addEventListener("click",()=>{detailsDialog.close();send("play-selected");});

  function showDetails(item) {
    if(!item)return notify("Selecciona un título del catálogo.");
    $("#details-title").textContent=item.title||"Sin título";
    $("#details-description").textContent=item.description||"Contenido disponible en tu catálogo de Xuper TV.";
    $("#details-type").textContent=typeLabel(item);
    $("#details-poster").style.backgroundImage=item.artwork?'url("'+item.artwork.replace(/["\\]/g,"")+'")':"";
    detailsDialog.showModal();
  }
  function typeLabel(item){if(item.kind==="channel")return "CANAL EN VIVO";if(item.kind==="asset")return "CATÁLOGO";if(item.kind==="episode")return "EPISODIO";return "EXPLORAR";}

  function makeFallback(kind,title){
    const wrap=document.createElement("div");
    wrap.className=kind==="category"?"category-art-fallback":"catalog-fallback";
    if(kind==="channel")wrap.append(svg(icons.live));else if(kind==="asset")wrap.textContent="X";else wrap.append(svg(icons.category));
    return wrap;
  }
  function setHero(item,index){
    const backdrop=$("#hero-backdrop"),artWrap=$("#hero-art-wrap"),art=$("#hero-art");
    const title=$("#hero-title"),desc=$("#hero-description"),meta=$("#hero-meta"),tag=$("#hero-kicker-text");
    if(!item){
      title.innerHTML="Tu entretenimiento.<br><em>Sin límites.</em>";
      desc.textContent="Canales en vivo, películas y series para encontrar siempre algo que ver.";
      meta.textContent="UNA EXPERIENCIA PARA DISFRUTAR";tag.textContent="BIENVENIDO A XUPER TV";
      backdrop.style.backgroundImage="";art.removeAttribute("src");artWrap.classList.remove("visible");
      $("#watch-label").textContent="Explorar ahora";$("#hero-index").textContent="XUPER TV";return;
    }
    title.textContent=item.title||"Xuper TV";
    desc.textContent=item.description||"Selecciona para abrir y comenzar a disfrutar.";
    meta.textContent=typeLabel(item);tag.textContent=item.kind==="channel"?"EN DIRECTO":"SELECCIÓN PARA TI";
    $("#watch-label").textContent=item.kind==="category"?"Explorar sección":"Ver ahora";
    $("#hero-index").textContent=String(index+1).padStart(2,"0")+"  /  "+String(items.length).padStart(2,"0");
    if(item.artwork){
      const safe=item.artwork.replace(/["\\]/g,"");
      backdrop.style.backgroundImage='linear-gradient(90deg,rgba(7,12,19,.22),rgba(7,12,19,.05)),url("'+safe+'")';
      art.onload=()=>artWrap.classList.add("visible");art.onerror=()=>artWrap.classList.remove("visible");art.src=item.artwork;
    }else{backdrop.style.backgroundImage="";art.removeAttribute("src");artWrap.classList.remove("visible");}
  }
  function createCard(item,index){
    const card=document.createElement("button");card.type="button";card.className="catalog-card"+(index===selectedIndex?" selected":"");
    card.dataset.kind=item.kind||"category";card.dataset.index=String(index);card.setAttribute("aria-label",(item.title||"Abrir contenido"));
    const art=document.createElement("div");art.className="catalog-art";
    if(item.artwork){
      const img=document.createElement("img");img.alt="";img.src=item.artwork;img.loading=index===selectedIndex?"eager":"lazy";img.decoding="async";img.fetchPriority=index===selectedIndex?"high":"auto";
      img.onerror=()=>{img.remove();art.append(makeFallback(item.kind,item.title));};art.append(img);
    }else art.append(makeFallback(item.kind,item.title));
    const title=document.createElement("span");title.className="catalog-title";title.textContent=item.title||"Sin título";card.append(art,title);
    if(item.kind==="channel"){const sub=document.createElement("span");sub.className="catalog-subtitle";sub.textContent="Disponible en TV en vivo";card.append(sub);}
    card.addEventListener("click",()=>{selectedIndex=index;send("select-item",{index});});
    return card;
  }
  function makeShelf(label,subtitle,entries){
    if(!entries.length)return null;
    const section=document.createElement("section");section.className="shelf";
    const heading=document.createElement("div");heading.className="shelf-heading";
    const left=document.createElement("div"),h=document.createElement("h2"),p=document.createElement("p"),count=document.createElement("span");
    h.textContent=label;p.textContent=subtitle;count.className="shelf-count";count.textContent=String(entries.length)+" títulos";
    left.append(h,p);heading.append(left,count);
    const rail=document.createElement("div");rail.className="card-rail";rail.setAttribute("aria-label",label);
    entries.forEach(({item,index})=>rail.append(createCard(item,index)));
    section.append(heading,rail);return section;
  }
  function renderQuick(entries){
    quickGrid.replaceChildren();
    entries.forEach(({item,index})=>{
      const card=document.createElement("button");card.className="quick-card";card.type="button";
      const key=/tv|vivo|canal/i.test(item.title)?"live":/pel[ií]cula|movie/i.test(item.title)?"movie":/serie/i.test(item.title)?"series":"category";
      const badge=document.createElement("span");badge.className="quick-icon";badge.append(svg(icons[key]));
      const copy=document.createElement("span");copy.className="quick-copy";
      const name=document.createElement("strong");name.textContent=item.title||"Explorar";
      const subtitle=document.createElement("span");subtitle.textContent=key==="live"?"Canales y televisión en directo":key==="movie"?"Historias para disfrutar":"Explora el catálogo disponible";
      copy.append(name,subtitle);const arrow=document.createElement("span");arrow.className="quick-arrow";arrow.textContent="›";
      card.append(badge,copy,arrow);card.addEventListener("click",()=>send("select-item",{index}));quickGrid.append(card);
    });
    quickSection.hidden=entries.length===0;
  }
  function renderCatalog(catalogItems=[],selected=0){
    items=Array.isArray(catalogItems)?catalogItems:[];selectedIndex=Math.max(0,Math.min(Number(selected)||0,Math.max(0,items.length-1)));
    railRoot.replaceChildren();
    if(!items.length){
      setHero(null,0);quickSection.hidden=true;
      const empty=document.createElement("div");empty.className="empty-state";
      const badge=document.createElement("div");badge.className="empty-symbol";badge.append(svg(icons.movie));
      const copy=document.createElement("div"),strong=document.createElement("strong"),hint=document.createElement("span");
      strong.textContent="No hay contenido para mostrar";hint.textContent="El catálogo aparecerá aquí cuando el servicio responda.";
      copy.append(strong,hint);empty.append(badge,copy);railRoot.append(empty);return;
    }
    const current=items[selectedIndex]||items[0];setHero(current,selectedIndex);
    const indexed=items.map((item,index)=>({item,index}));
    const quick=activePage==="Inicio"?indexed.filter(x=>x.item.kind==="category"&&!/favorito|apoyar|continuar/i.test(x.item.title||"")):[];
    renderQuick(quick);
    const channels=indexed.filter(x=>x.item.kind==="channel");
    const assets=indexed.filter(x=>x.item.kind==="asset");
    const episodes=indexed.filter(x=>x.item.kind==="episode");
    const categories=indexed.filter(x=>x.item.kind==="category");
    if(quick.length && activePage==="Inicio"){
      const section=makeShelf("Tu catálogo","Elige una sección para comenzar",indexed.filter(x=>x.item.kind==="category"&&/favorito|continuar/i.test(x.item.title||"")));
      if(section)railRoot.append(section);
    }
    if(channels.length){const s=makeShelf("Canales disponibles","La programación que puedes ver ahora",channels);if(s)railRoot.append(s);}
    if(assets.length){const label=activePage==="Series"?"Series para descubrir":activePage==="Películas"?"Películas para disfrutar":"Recomendados para ti";const s=makeShelf(label,"Explora los títulos de tu catálogo",assets);if(s)railRoot.append(s);}
    if(episodes.length){const s=makeShelf("Episodios","Selecciona un episodio para reproducir",episodes);if(s)railRoot.append(s);}
    if(categories.length && !quick.length){const s=makeShelf(activePage==="TV en vivo"?"Categorías de TV":activePage==="Películas"?"Categorías de películas":activePage==="Series"?"Categorías de series":"Explora por categorías","Selecciona una categoría para ver su contenido",categories);if(s)railRoot.append(s);}
    if(!railRoot.children.length && !quick.length){
      const s=makeShelf("Explora el catálogo","Selecciona un elemento para continuar",indexed);if(s)railRoot.append(s);
    }
  }

  document.addEventListener("keydown",e=>{
    if((e.ctrlKey||e.metaKey)&&e.key.toLowerCase()==="k"){e.preventDefault();openSearch();}
    if(e.key==="Escape"){if(searchDialog.open||detailsDialog.open)return;send("back");}
    if(["ArrowLeft","ArrowRight","ArrowUp","ArrowDown"].includes(e.key)&&!searchDialog.open&&!detailsDialog.open){
      const active=document.activeElement;if(active?.tagName==="INPUT")return;
      const buttons=$$("button:not(:disabled):not([hidden])").filter(b=>b.offsetWidth&&b.offsetHeight);
      const idx=buttons.indexOf(active);if(idx>=0){e.preventDefault();const delta=e.key==="ArrowLeft"?-1:e.key==="ArrowRight"?1:e.key==="ArrowUp"?-4:4;buttons[Math.max(0,Math.min(buttons.length-1,idx+delta))]?.focus();}
    }
  });
  if(window.chrome?.webview){
    window.chrome.webview.addEventListener("message",event=>{
      const msg=event.data||{};
      if(msg.type==="catalog")renderCatalog(msg.items,msg.selected);
      if(msg.type==="page")setPage(msg.page||"Inicio",false);
      if(msg.type==="status")setStatus(msg.text);
    });
  }
  window.xuperUI=Object.freeze({renderCatalog,setPage,setStatus});
  window.xuperReady?.();
})();