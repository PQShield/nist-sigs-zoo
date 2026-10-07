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
