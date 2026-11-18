#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum Color {
    White = 0,
    Black = 1,
}
const MAX_COLOR: usize = 2;

// Must be in same order as the Color values!
const ALL_COLORS: [Color; MAX_COLOR] = [Color::White, Color::Black];

#[derive(Default)]
pub struct ByColor<T>([T; MAX_COLOR]);

impl<T> ByColor<T> {
    pub fn iter(&self) -> impl Iterator<Item = (Color, &T)> {
        ALL_COLORS.into_iter().zip(self.0.iter())
    }
}

impl<T> std::ops::Index<Color> for ByColor<T> {
    type Output = T;
    fn index(&self, c: Color) -> &Self::Output {
        &self.0[c as usize]
    }
}

impl<T> std::ops::IndexMut<Color> for ByColor<T> {
    fn index_mut(&mut self, c: Color) -> &mut Self::Output {
        &mut self.0[c as usize]
    }
}
