export interface Point {
	x: number;
	y: number;
}

/** Pointer travel (px) above which a mousedown→click is a pan, not a click. */
export const CLICK_SLOP_PX = 5;

/** The scheme-page href of a clicked chart datum, or null for drags, background and non-links. */
export function chartClickHref(datum: unknown, down: Point | null, up: Point): string | null {
	if (down && Math.hypot(up.x - down.x, up.y - down.y) > CLICK_SLOP_PX) return null;
	if (!datum || typeof datum !== 'object') return null;
	const href = (datum as { href?: unknown }).href;
	return typeof href === 'string' && href.length > 0 ? href : null;
}

interface NavView {
	addEventListener(type: 'click', handler: (event: Event, item?: { datum?: unknown } | null) => void): unknown;
}

/**
 * Make chart points navigable. Vega preventDefaults `pointerdown`, which suppresses the
 * compatibility mouse events, so the press position is taken from a capturing `pointerdown`
 * listener on the container. Returns a cleanup function.
 */
export function attachChartNav(container: HTMLElement, view: NavView, navigate: (href: string) => void): () => void {
	let down: Point | null = null;
	const onDown = (e: PointerEvent) => {
		down = { x: e.clientX, y: e.clientY };
	};
	container.addEventListener('pointerdown', onDown, { capture: true });
	view.addEventListener('click', (event, item) => {
		const e = event as MouseEvent;
		const href = chartClickHref(item?.datum, down, { x: e.clientX, y: e.clientY });
		down = null;
		if (href) navigate(href);
	});
	return () => container.removeEventListener('pointerdown', onDown, { capture: true });
}
