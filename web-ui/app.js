(() => {
  "use strict";

  const $ = (id) => document.getElementById(id);
  const navItems = Array.from(document.querySelectorAll(".nav-item[data-page]"));
  const rail = $("card-rail");
  const detailsDialog = $("details-dialog");
  const toast = $("toast");
  let activePage = "Inicio";
  let items = [];
  let selectedIndex = 0;
  let detailIndex = 0;
  let toastTimer = 0;

  const pageCopy = {
    "Inicio": ["DESCUBRE", "Explora tu catálogo", "Contenido recibido desde el servicio de Xuper TV."],
    "TV en vivo": ["TELEVISIÓN", "Canales y categorías", "Selecciona una categoría o un canal disponible."],
    "Películas": ["CINE", "Catálogo de películas", "Películas disponibles en tu servicio."],
    "Series": ["SERIES", "Explora tus series", "Series y episodios disponibles en tu servicio."],
    "Favoritos": ["TU BIBLIOTECA", "Mis favoritos", "Canales que has guardado desde Xuper TV."],
    "Apoyar": ["COMUNIDAD", "Apoya Xuper TV", "Gracias por apoyar el desarrollo de este proyecto."]
  };

  function notify(message) {
    toast.textContent = String(message || "");
    toast.classList.add("show");
    window.clearTimeout(toastTimer);
    toastTimer = window.setTimeout(() => toast.classList.remove("show"), 2400);
  }

  function send(type, payload = {}) {
    const calls = {
      navigate: () => window.xuperNavigate?.(payload.page || activePage),
      "play-selected": () => window.xuperPlay?.(),
      favorite: () => window.xuperFavorite?.(),
      "search-query": () => window.xuperSearch?.(payload.query || ""),
      "select-item": () => window.xuperSelectItem?.(Number(payload.index || 0)),
      back: () => window.xuperBack?.()
    };
    if (calls[type] && (window.xuperNavigate || window.xuperPlay || window.xuperSearch)) {
      calls[type]();
    } else {
      notify("No se pudo conectar con el motor de Xuper TV.");
    }
  }

  function setPage(page, notifyNative = true) {
    activePage = pageCopy[page] ? page : "Inicio";
    navItems.forEach((item) => {
      const active = item.dataset.page === activePage;
      item.classList.toggle("active", active);
      if (active) item.setAttribute("aria-current", "page");
      else item.removeAttribute("aria-current");
    });
    $("page-title").textContent = activePage;
    const copy = pageCopy[activePage];
    $("section-kicker").textContent = copy[0];
    $("shelf-title").textContent = copy[1];
    $("shelf-description").textContent = copy[2];
    rail.classList.toggle("grid-mode", activePage !== "Inicio");
    $("carousel-controls").hidden = activePage !== "Inicio";
    if (notifyNative) send("navigate", { page: activePage });
  }

  function setStatus(message) {
    const value = String(message || "Listo");
    $("service-status").textContent = value;
    const pill = $("status-pill");
    const lower = value.toLocaleLowerCase();
    pill.dataset.state = /error|no se pudo|fall[oó]|sin conexi[oó]n|no disponible/.test(lower)
      ? "error"
      : /conectando|cargando|buscando|preparando|consultando/.test(lower) ? "loading" : "ready";
    pill.title = value;
  }

  function kindLabel(kind) {
    const labels = {
      channel: "Canal", episode: "Episodio", asset: "Contenido",
      media: "Contenido", category: "Categoría", section: "Colección",
      "vod-category": "Categoría", "vod-page": "Página",
      "search-page": "Resultados", "live-section": "TV en vivo",
      "search-action": "Buscar", donation: "Apoyar Xuper",
      favorites: "Favoritos", resume: "Continuar viendo"
    };
    return labels[kind] || "Contenido";
  }

  function kindClass(kind) {
    return String(kind || "media").toLowerCase().replace(/[^a-z0-9-]/g, "");
  }

  function makePoster(item) {
    const poster = document.createElement("div");
    poster.className = "media-poster";
    if (item.artwork) {
      const img = document.createElement("img");
      img.src = item.artwork;
      img.alt = "";
      img.loading = "lazy";
      img.referrerPolicy = "no-referrer";
      img.addEventListener("error", () => img.remove(), { once: true });
      poster.append(img);
    } else {
      const fallback = document.createElement("span");
      fallback.className = "poster-fallback";
      fallback.textContent = item.kind === "channel" ? "▶" : "X";
      poster.append(fallback);
    }
    const badge = document.createElement("span");
    badge.className = "media-badge";
    badge.textContent = kindLabel(item.kind).toLocaleUpperCase();
    poster.append(badge);
    return poster;
  }

  function makeCard(item, index) {
    const card = document.createElement("button");
    card.type = "button";
    card.className = "media-card kind-" + kindClass(item.kind);
    if (index === selectedIndex) card.classList.add("selected");
    card.style.animationDelay = Math.min(index * 24, 240) + "ms";
    card.setAttribute("aria-label", (item.title || "Abrir contenido") + " · " + kindLabel(item.kind));
    card.append(makePoster(item));

    const info = document.createElement("span");
    info.className = "media-info";
    const type = document.createElement("span");
    type.className = "media-type";
    type.textContent = kindLabel(item.kind);
    const title = document.createElement("strong");
    title.className = "media-title";
    title.textContent = item.title || "Sin título";
    info.append(type, title);
    if (item.description) {
      const description = document.createElement("span");
      description.className = "media-description";
      description.textContent = item.description;
      info.append(description);
    }
    card.append(info);
    card.addEventListener("click", () => send("select-item", { index }));
    return card;
  }

  function showHero() {
    const current = items[selectedIndex] || items[0] || null;
    const art = $("hero-artwork");
    if (!current) {
      $("hero-title").textContent = "Todo tu entretenimiento.";
      $("hero-description").textContent = "Explora el catálogo conectado a tu cuenta y selecciona lo que quieres disfrutar.";
      $("hero-kind").textContent = "Xuper TV";
      $("hero-context").textContent = "Contenido del servicio";
      $("hero-tag-text").textContent = "TU CATÁLOGO";
      $("hero-open-label").textContent = "Abrir selección";
      $("hero-open").disabled = true;
      $("hero-details").disabled = true;
      $("hero-favorite").hidden = true;
      art.removeAttribute("src");
      art.hidden = true;
      return;
    }

    $("hero-title").textContent = current.title || "Xuper TV";
    $("hero-description").textContent = current.description || "Selecciona este elemento para abrirlo en Xuper TV.";
    $("hero-kind").textContent = kindLabel(current.kind);
    $("hero-context").textContent = activePage;
    $("hero-tag-text").textContent = current.kind === "channel" ? "CANAL DISPONIBLE" :
      current.kind === "live-section" ? "TELEVISIÓN EN VIVO" : "SELECCIÓN DEL CATÁLOGO";
    $("hero-open-label").textContent = ["category", "section", "vod-category", "vod-page", "search-page", "live-section", "favorites", "resume", "search-action", "donation"].includes(current.kind)
      ? "Abrir sección" : "Reproducir";
    $("hero-open").disabled = false;
    $("hero-details").disabled = false;
    $("hero-favorite").hidden = current.kind !== "channel";
    if (current.artwork) {
      art.src = current.artwork;
      art.hidden = false;
    } else {
      art.removeAttribute("src");
      art.hidden = true;
    }
  }

  function renderCatalog(catalogItems = [], selected = 0, meta = {}) {
    items = Array.isArray(catalogItems) ? catalogItems : [];
    const rawSelected = Number(selected);
    selectedIndex = Number.isFinite(rawSelected)
      ? Math.max(0, Math.min(Math.trunc(rawSelected), Math.max(0, items.length - 1))) : 0;
    rail.replaceChildren();
    if (items.length) {
      const fragment = document.createDocumentFragment();
      items.forEach((item, index) => fragment.append(makeCard(item || {}, index)));
      rail.append(fragment);
    }
    $("empty-state").hidden = items.length !== 0;
    $("empty-title").textContent = activePage === "Favoritos" ? "Todavía no tienes favoritos" : "No hay elementos para mostrar";
    $("empty-description").textContent = activePage === "Favoritos"
      ? "Los canales que marques como favoritos aparecerán aquí."
      : "El servicio no ha enviado contenido para esta sección.";
    if (meta && meta.title && activePage === "Inicio") $("shelf-title").textContent = meta.title;
    showHero();
  }

  function openDetails(index) {
    const item = items[index];
    if (!item) return;
    detailIndex = index;
    $("details-title").textContent = item.title || "Sin título";
    $("details-kind").textContent = kindLabel(item.kind).toLocaleUpperCase();
    $("details-description").textContent = item.description || "No hay una descripción adicional para este elemento.";
    $("details-favorite").hidden = item.kind !== "channel";
    detailsDialog.showModal();
  }

  navItems.forEach((item) => item.addEventListener("click", () => setPage(item.dataset.page)));
  document.querySelector("[data-home]").addEventListener("click", (event) => {
    event.preventDefault();
    setPage("Inicio");
  });

  $("search-form").addEventListener("submit", (event) => {
    event.preventDefault();
    const query = $("search-input").value.trim();
    if (!query) return;
    send("search-query", { query });
    setStatus("Buscando: " + query);
    $("search-input").blur();
  });

  $("hero-open").addEventListener("click", () => send("play-selected"));
  $("hero-details").addEventListener("click", () => openDetails(selectedIndex));
  $("hero-favorite").addEventListener("click", () => send("favorite"));
  $("details-open").addEventListener("click", () => {
    detailsDialog.close();
    send("select-item", { index: detailIndex });
  });
  $("details-favorite").addEventListener("click", () => {
    detailsDialog.close();
    send("favorite");
  });

  $("scroll-left").addEventListener("click", () => rail.scrollBy({ left: -520, behavior: "smooth" }));
  $("scroll-right").addEventListener("click", () => rail.scrollBy({ left: 520, behavior: "smooth" }));

  function focusCard(index) {
    const cards = Array.from(rail.querySelectorAll(".media-card"));
    if (!cards.length) return;
    const target = Math.max(0, Math.min(index, cards.length - 1));
    cards[target].focus();
    cards[target].scrollIntoView({ block: "nearest", inline: "nearest", behavior: "smooth" });
  }

  document.addEventListener("keydown", (event) => {
    if ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === "k") {
      event.preventDefault();
      $("search-input").focus();
      return;
    }
    if (event.key === "Escape") {
      if (detailsDialog.open) detailsDialog.close();
      else send("back");
      return;
    }

    const cards = Array.from(rail.querySelectorAll(".media-card"));
    const cardIndex = cards.indexOf(document.activeElement);
    const navIndex = navItems.indexOf(document.activeElement);
    const arrows = ["ArrowLeft", "ArrowRight", "ArrowUp", "ArrowDown"];
    if (!arrows.includes(event.key)) return;

    if (cardIndex >= 0) {
      let delta = 0;
      if (event.key === "ArrowLeft") delta = -1;
      if (event.key === "ArrowRight") delta = 1;
      if (event.key === "ArrowUp" || event.key === "ArrowDown") {
        if (!rail.classList.contains("grid-mode")) return;
        const columns = Math.max(1, getComputedStyle(rail).gridTemplateColumns.split(" ").length);
        delta = event.key === "ArrowUp" ? -columns : columns;
      }
      if (delta) {
        event.preventDefault();
        focusCard(cardIndex + delta);
      }
      return;
    }

    if (navIndex >= 0 && (event.key === "ArrowLeft" || event.key === "ArrowRight")) {
      event.preventDefault();
      const next = Math.max(0, Math.min(navIndex + (event.key === "ArrowLeft" ? -1 : 1), navItems.length - 1));
      navItems[next].focus();
      return;
    }

    if ((navIndex >= 0 && event.key === "ArrowDown") ||
        (!navIndex && event.key === "ArrowDown" && document.activeElement === document.body)) {
      if (cards.length) {
        event.preventDefault();
        focusCard(0);
      }
    }
    if (navIndex >= 0 && event.key === "ArrowUp") {
      event.preventDefault();
      $("search-input").focus();
    }
  });

  if (window.chrome?.webview) {
    window.chrome.webview.addEventListener("message", (event) => {
      const message = event.data || {};
      if (message.type === "catalog") renderCatalog(message.items, message.selected, message.meta);
      if (message.type === "page") setPage(message.page || "Inicio", false);
      if (message.type === "status") setStatus(message.text);
    });
  }

  window.xuperUI = Object.freeze({ renderCatalog, setPage, setStatus });
  window.xuperReady?.();
})();