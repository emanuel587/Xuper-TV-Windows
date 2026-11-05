(() => {
  "use strict";
  const $ = (id) => document.getElementById(id);
  const navItems = Array.from(document.querySelectorAll(".nav-item[data-page]"));
  const views = Array.from(document.querySelectorAll(".view[data-view]"));
  const rail = $("card-rail");
  const detailsDialog = $("details-dialog");
  const toast = $("toast");
  let activePage = "Inicio", items = [], selectedIndex = 0, detailIndex = 0, toastTimer = 0;

  const pageIds = {"Inicio":"view-home","TV en vivo":"view-live","Películas":"view-movies","Series":"view-series","Favoritos":"view-favorites","Apoyar":"view-support"};
  const pageCopy = {
    "Inicio":["PARA TI","Descubre Xuper TV","Selección disponible desde tu servicio."],
    "TV en vivo":["TELEVISIÓN","Canales y categorías","Selecciona una categoría o un canal disponible."],
    "Películas":["CINE","Explorar películas","Películas disponibles en tu servicio."],
    "Series":["SERIES","Explorar series","Series y episodios disponibles en tu servicio."],
    "Favoritos":["TU BIBLIOTECA","Mis favoritos","Elementos que has guardado desde Xuper TV."],
    "Apoyar":["COMUNIDAD","Apoya Xuper TV","Gracias por apoyar este proyecto."]
  };
  const kindNames = {channel:"Canal",episode:"Episodio",asset:"Contenido",media:"Contenido",category:"Categoría",section:"Colección","vod-category":"Categoría","vod-page":"Página","search-page":"Resultados","live-section":"TV en vivo","search-action":"Buscar",donation:"Apoyar Xuper",favorites:"Favoritos",resume:"Continuar viendo"};

  function notify(message) {
    toast.textContent = String(message || "");
    toast.classList.add("show"); window.clearTimeout(toastTimer);
    toastTimer = window.setTimeout(() => toast.classList.remove("show"), 2400);
  }
  function send(type, payload = {}) {
    const calls = {
      navigate:() => window.xuperNavigate?.(payload.page || activePage),
      "play-selected":() => window.xuperPlay?.(),
      favorite:() => window.xuperFavorite?.(),
      "search-query":() => window.xuperSearch?.(payload.query || ""),
      "select-item":() => window.xuperSelectItem?.(Number(payload.index || 0)),
      back:() => window.xuperBack?.()
    };
    if (calls[type] && (window.xuperNavigate || window.xuperPlay || window.xuperSearch)) calls[type]();
    else notify("No se pudo conectar con el motor de Xuper TV.");
  }
  function setPage(page, notifyNative = true) {
    activePage = pageCopy[page] ? page : "Inicio";
    navItems.forEach((node) => {
      const active = node.dataset.page === activePage;
      node.classList.toggle("active", active);
      if (active) node.setAttribute("aria-current","page"); else node.removeAttribute("aria-current");
    });
    views.forEach((view) => {
      const active = view.dataset.view === activePage;
      view.hidden = !active; view.classList.toggle("active",active);
    });
    const copy = pageCopy[activePage];
    $("section-kicker").textContent = copy[0]; $("shelf-title").textContent = copy[1]; $("shelf-description").textContent = copy[2];
    if (notifyNative) send("navigate",{page:activePage});
  }
  function setStatus(message) {
    const value = String(message || "Listo"); $("service-status").textContent = value;
    const pill = $("status-pill"), lower = value.toLocaleLowerCase();
    pill.dataset.state = /error|no se pudo|fall[oó]|sin conexi[oó]n|no disponible/.test(lower) ? "error" :
      /conectando|cargando|buscando|preparando|consultando/.test(lower) ? "loading" : "ready";
    pill.title = value;
  }
  function kindLabel(kind) { return kindNames[kind] || "Contenido"; }
  function isSection(item) { return ["category","section","vod-category","vod-page","search-page","live-section","search-action","favorites","resume","donation"].includes(item.kind); }

  function makeArtwork(item, className = "poster-art") {
    const wrap = document.createElement("div"); wrap.className = className;
    if (item.artwork) {
      const img = document.createElement("img"); img.src = item.artwork; img.alt = ""; img.loading = "lazy"; img.referrerPolicy = "no-referrer";
      img.addEventListener("error",() => img.remove(),{once:true}); wrap.append(img);
    } else {
      const fallback = document.createElement("span"); fallback.className = "art-fallback"; fallback.textContent = item.kind === "channel" ? "▶" : "X"; wrap.append(fallback);
    }
    return wrap;
  }
  function makeCard(item,index,variant="poster-card") {
    const card = document.createElement("button"); card.type = "button"; card.className = variant + " kind-" + String(item.kind || "media").replace(/[^a-z0-9-]/gi,"");
    card.setAttribute("aria-label",(item.title || "Abrir contenido") + " · " + kindLabel(item.kind));
    card.append(makeArtwork(item));
    const copy = document.createElement("span"); copy.className = "poster-copy";
    const type = document.createElement("small"); type.textContent = kindLabel(item.kind).toUpperCase();
    const title = document.createElement("strong"); title.textContent = item.title || "Sin título";
    copy.append(type,title);
    if (item.description) { const desc = document.createElement("span"); desc.className="poster-description"; desc.textContent=item.description; copy.append(desc); }
    card.append(copy); card.addEventListener("click",() => send("select-item",{index})); return card;
  }
  function setEmpty(id,empty) { const node=$(id); if(node) node.hidden=!empty; }
  function showHero() {
    const current = items[selectedIndex] || items[0] || null, art=$("hero-artwork");
    if (!current) {
      $("hero-title").innerHTML = "Todo lo que te gusta.<br>En un solo lugar.";
      $("hero-description").textContent="Explora el contenido disponible en tu servicio Xuper TV.";
      $("hero-kind").textContent="XUPER TV"; $("hero-context").textContent="Descubre tu catálogo";
      $("hero-tag-text").textContent="TU ENTRETENIMIENTO"; $("hero-open-label").textContent="Explorar ahora";
      $("hero-open").disabled=true; $("hero-details").disabled=true; $("hero-favorite").hidden=true;
      art.removeAttribute("src"); art.hidden=true; return;
    }
    $("hero-title").textContent=current.title || "Xuper TV";
    $("hero-description").textContent=current.description || "Selecciona este elemento para abrirlo en Xuper TV.";
    $("hero-kind").textContent=kindLabel(current.kind).toUpperCase();
    $("hero-context").textContent=activePage;
    $("hero-tag-text").textContent=current.kind==="channel" ? "CANAL DISPONIBLE" : "SELECCIÓN DEL CATÁLOGO";
    $("hero-open-label").textContent=isSection(current) ? "Abrir sección" : "Reproducir";
    $("hero-open").disabled=false; $("hero-details").disabled=false; $("hero-favorite").hidden=current.kind!=="channel";
    if(current.artwork){art.src=current.artwork;art.hidden=false;}else{art.removeAttribute("src");art.hidden=true;}
  }
  function renderLive() {
    const categories=items.map((item,index)=>({item,index})).filter(x=>["category","live-section"].includes(x.item.kind));
    const channels=items.map((item,index)=>({item,index})).filter(x=>x.item.kind==="channel");
    const categoryList=$("live-category-list"), channelList=$("live-channel-list");
    categoryList.replaceChildren(); channelList.replaceChildren();
    categories.forEach(({item,index})=>{
      const button=document.createElement("button"); button.className="live-category"; button.type="button";
      button.textContent=item.title || "Categoría"; button.addEventListener("click",()=>send("select-item",{index})); categoryList.append(button);
    });
    channels.forEach(({item,index})=>{
      const button=document.createElement("button"); button.className="live-channel"+(index===selectedIndex?" selected":""); button.type="button";
      button.append(makeArtwork(item,"channel-logo"));
      const info=document.createElement("span"); info.className="channel-copy";
      const title=document.createElement("strong"); title.textContent=item.title || "Canal";
      const kind=document.createElement("small"); kind.textContent=kindLabel(item.kind).toUpperCase(); info.append(title,kind);
      const play=document.createElement("span"); play.className="channel-arrow"; play.textContent="›";
      button.append(info,play); button.addEventListener("click",()=>send("select-item",{index})); channelList.append(button);
    });
    $("live-category-count").textContent=String(categories.length);
    $("live-channel-count").textContent=String(channels.length);
    setEmpty("live-empty",channels.length===0);
    const current=items[selectedIndex] && items[selectedIndex].kind==="channel" ? items[selectedIndex] : channels.length ? channels[0].item : null;
    $("live-detail-title").textContent=current?.title || "Selecciona un canal";
    $("live-detail-kind").textContent=current ? "CANAL DISPONIBLE" : "CANAL SELECCIONADO";
    $("live-detail-description").textContent=current?.description || "La información disponible del canal aparecerá aquí.";
    $("live-play").disabled=!current;
    const image=$("live-preview-artwork");
    if(current?.artwork){image.src=current.artwork;image.hidden=false;}else{image.removeAttribute("src");image.hidden=true;}
  }
  function renderGrid(targetId,emptyId,filter,countId) {
    const target=$(targetId); if(!target)return; target.replaceChildren();
    const list=items.map((item,index)=>({item,index})).filter(x=>filter(x.item));
    list.forEach(({item,index})=>target.append(makeCard(item,index,"library-card")));
    setEmpty(emptyId,list.length===0);
    if(countId)$(countId).textContent=list.length+" elementos";
  }
  function renderCatalog(catalogItems=[],selected=0,meta={}) {
    items=Array.isArray(catalogItems)?catalogItems:[];
    const raw=Number(selected); selectedIndex=Number.isFinite(raw)?Math.max(0,Math.min(Math.trunc(raw),Math.max(0,items.length-1))):0;
    rail.replaceChildren();
    items.forEach((item,index)=>rail.append(makeCard(item||{},index)));
    setEmpty("empty-state",items.length===0);
    $("empty-title").textContent=activePage==="Favoritos"?"Todavía no tienes favoritos":"No hay elementos para mostrar";
    $("empty-description").textContent=activePage==="Favoritos"?"Los elementos que marques como favoritos aparecerán aquí.":"El servicio no ha enviado contenido para esta sección.";
    if(meta?.title && activePage==="Inicio")$("shelf-title").textContent=meta.title;
    showHero(); renderLive();
    renderGrid("movies-grid","movies-empty",item=>item.kind!=="channel" && !isSection(item),"movies-count");
    renderGrid("series-grid","series-empty",item=>item.kind==="episode" || item.kind==="media" || item.kind==="asset","series-count");
    renderGrid("favorites-grid","favorites-empty",item=>item.kind==="channel" || item.kind==="media" || item.kind==="asset");
  }
  function openDetails(index) {
    const item=items[index]; if(!item)return; detailIndex=index;
    $("details-title").textContent=item.title || "Sin título";
    $("details-kind").textContent=kindLabel(item.kind).toUpperCase();
    $("details-description").textContent=item.description || "No hay una descripción adicional para este elemento.";
    $("details-favorite").hidden=item.kind!=="channel"; detailsDialog.showModal();
  }
  navItems.forEach(node=>node.addEventListener("click",()=>setPage(node.dataset.page)));
  document.querySelectorAll("[data-shortcut]").forEach(node=>node.addEventListener("click",()=>setPage(node.dataset.shortcut)));
  document.querySelectorAll("[data-page]").forEach(node=>{if(!node.classList.contains("nav-item"))node.addEventListener("click",()=>setPage(node.dataset.page));});
  document.querySelector("[data-home]").addEventListener("click",event=>{event.preventDefault();setPage("Inicio");});
  $("search-form").addEventListener("submit",event=>{event.preventDefault();const query=$("search-input").value.trim();if(!query)return;send("search-query",{query});setStatus("Buscando: "+query);$("search-input").blur();});
  $("hero-open").addEventListener("click",()=>send("play-selected"));
  $("hero-details").addEventListener("click",()=>openDetails(selectedIndex));
  $("hero-favorite").addEventListener("click",()=>send("favorite"));
  $("live-play").addEventListener("click",()=>send("play-selected"));
  $("details-open").addEventListener("click",()=>{detailsDialog.close();send("select-item",{index:detailIndex});});
  $("details-favorite").addEventListener("click",()=>{detailsDialog.close();send("favorite");});
  $("scroll-left").addEventListener("click",()=>rail.scrollBy({left:-520,behavior:"smooth"}));
  $("scroll-right").addEventListener("click",()=>rail.scrollBy({left:520,behavior:"smooth"}));

  function focusCard(index) {
    const cards=Array.from(document.querySelectorAll(".view.active button:not(:disabled)")).filter(node=>!node.closest(".main-nav"));
    if(!cards.length)return; cards[Math.max(0,Math.min(index,cards.length-1))].focus();
  }
  document.addEventListener("keydown",event=>{
    if((event.ctrlKey||event.metaKey)&&event.key.toLowerCase()==="k"){event.preventDefault();$("search-input").focus();return;}
    if(event.key==="Escape"){if(detailsDialog.open)detailsDialog.close();else send("back");return;}
    if(!["ArrowLeft","ArrowRight","ArrowUp","ArrowDown"].includes(event.key))return;
    const active=document.activeElement, navIndex=navItems.indexOf(active);
    if(navIndex>=0&&(event.key==="ArrowLeft"||event.key==="ArrowRight")){event.preventDefault();navItems[Math.max(0,Math.min(navIndex+(event.key==="ArrowLeft"?-1:1),navItems.length-1))].focus();return;}
    if((navIndex>=0&&event.key==="ArrowDown")||(active===document.body&&event.key==="ArrowDown")){event.preventDefault();focusCard(0);}
  });
  if(window.chrome?.webview)window.chrome.webview.addEventListener("message",event=>{
    const message=event.data||{};
    if(message.type==="catalog")renderCatalog(message.items,message.selected,message.meta);
    if(message.type==="page")setPage(message.page||"Inicio",false);
    if(message.type==="status")setStatus(message.text);
  });
  window.xuperUI=Object.freeze({renderCatalog,setPage,setStatus});
  window.xuperReady?.();
})();